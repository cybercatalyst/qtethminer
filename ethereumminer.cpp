///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    This file is part of QtEthminer.                                       //
//    Copyright (C) 2015-2016 Jacob Dawid, jacob@omg-it.works                //
//                                                                           //
//    QtEthminer is free software: you can redistribute it and/or modify     //
//    it under the terms of the GNU Affero General Public License as         //
//    published by the Free Software Foundation, either version 3 of the     //
//    License, or (at your option) any later version.                        //
//                                                                           //
//    QtEthminer is distributed in the hope that it will be useful,          //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of         //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
//    GNU Affero General Public License for more details.                    //
//                                                                           //
//    You should have received a copy of the GNU General Public License      //
//    along with QtEthminer. If not, see <http://www.gnu.org/licenses/>.     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "ethereumminer.h"

// Qt includes
#include <QDebug>

EthereumMiner::EthereumMiner(QObject *parent) :
    QObject(parent) {
    _ethereumProtocol = new EthereumProtocol(this);

    // When losing connection, handle this.
    connect(_ethereumProtocol->stratumClient(), SIGNAL(disconnectedFromServer()), this, SLOT(handleDisconnect()));
    // Once we login to the server, we want to login in right after that.
    connect(_ethereumProtocol->stratumClient(), SIGNAL(connectedToServer()), this, SLOT(login()));
    // Once we have logged in, query for work.
    connect(_ethereumProtocol, SIGNAL(eth_login(bool)), _ethereumProtocol, SLOT(eth_getWork()));
    // Evertime we receive a work package, work on it.
    connect(_ethereumProtocol, SIGNAL(eth_getWork(QString,QString,QString)), this, SLOT(processWorkPackage(QString,QString,QString)));
}

EthereumMiner::~EthereumMiner() {
}

QThread* EthereumMiner::processInBackground() {
    QThread *workerThread = new QThread();
    moveToThread(workerThread);
    workerThread->start();
    return workerThread;
}

void EthereumMiner::startMining() {
    emit currentStep(Starting);
    QMetaObject::invokeMethod(this, "begin");
}

void EthereumMiner::stopMining() {
    emit currentStep(Halted);
    QMetaObject::invokeMethod(this, "halt");
}

void EthereumMiner::restartMining() {
    stopMining();
    startMining();
}

void EthereumMiner::handleDisconnect() {
    // In case we are mining, we obviously want to reconnect.
    if(_powFarm.isMining()) {
        connectToServer();
    }
}

void EthereumMiner::connectToServer() {
    emit currentStep(Connecting);
    _ethereumProtocol->stratumClient()->connectToServer(_configuration.server, _configuration.port);
}

void EthereumMiner::login() {
    emit currentStep(LoggingIn);
    _ethereumProtocol->eth_login(_configuration.username, _configuration.password);
}

int EthereumMiner::hashrate() {
    dev::eth::WorkingProgress wp = _powFarm.miningProgress();
    _powFarm.resetMiningProgress();
    return (int)wp.rate();
}

void EthereumMiner::saveSettings(QSettings& settings) {
    switch(_configuration.minerType) {
        case CPUMiner:
            settings.setValue("minerType", "CPUMiner");
        break;
        case OpenCLMiner:
            settings.setValue("minerType", "OpenCLMiner");
        break;
    }

    settings.setValue("openclPlatform", _configuration.openclPlatform);
    settings.setValue("openclDevice", _configuration.openclDevice);
    settings.setValue("maxMiningThreads", _configuration.maxMiningThreads);
    settings.setValue("currentBlock", (int)_configuration.currentBlock);
    settings.setValue("recognizeCPUAsOpenCLDevice", _configuration.recognizeCPUAsOpenCLDevice);
    settings.setValue("extraGPUMemory", _configuration.extraGPUMemory);
    settings.setValue("precomputeNextDAG", _configuration.precomputeNextDAG);
    settings.setValue("username", _configuration.username);
    settings.setValue("password", _configuration.password);
    settings.setValue("server", _configuration.server);
    settings.setValue("port", _configuration.port);
    settings.setValue("globalWorkSizeMultiplier", _configuration.globalWorkSizeMultiplier);
    settings.setValue("localWorkSize", _configuration.localWorkSize);
    settings.setValue("msPerBatch", _configuration.msPerBatch);
    settings.sync();
}

void EthereumMiner::loadSettings(QSettings& settings) {
    settings.sync();

    QString minerTypeString = settings.value("minerType", "CPUMiner").toString();
    if(minerTypeString == "CPUMiner") {
        _configuration.minerType = CPUMiner;
    } else
    if(minerTypeString == "OpenCLMiner") {
        _configuration.minerType = OpenCLMiner;
    }

    _configuration.openclPlatform = settings.value("openclPlatform", 0).toInt();
    _configuration.openclDevice = settings.value("openclDevice", 0).toInt();
    _configuration.maxMiningThreads = settings.value("maxMiningThreads", UINT_MAX).toInt();
    _configuration.currentBlock = settings.value("currentBlock", 0).toInt();
    _configuration.recognizeCPUAsOpenCLDevice = settings.value("recognizeCPUAsOpenCLDevice", false).toBool();
    _configuration.extraGPUMemory = settings.value("extraGPUMemory", 64000000).toInt();
    _configuration.precomputeNextDAG = settings.value("precomputeNextDAG", true).toBool();
    _configuration.username = settings.value("username", "0x0.rig1").toString();
    _configuration.password = settings.value("password", "unused").toString();
    _configuration.server = settings.value("server", "eu1.ethermine.org").toString();
    _configuration.port = settings.value("port", 4444).toInt();
    _configuration.globalWorkSizeMultiplier = settings.value("globalWorkSizeMultiplier", ethash_cl_miner::c_defaultGlobalWorkSizeMultiplier).toInt();
    _configuration.localWorkSize = settings.value("localWorkSize", ethash_cl_miner::c_defaultLocalWorkSize).toInt();
    _configuration.msPerBatch = settings.value("msPerBatch", ethash_cl_miner::c_defaultMSPerBatch).toInt();
}

void EthereumMiner::processWorkPackage(QString headerHash, QString seedHash, QString boundary) {
    emit currentStep(Mining);
    emit receivedWorkPackage(headerHash, seedHash, boundary);

    dev::h256 newHeaderHash(headerHash.toStdString());
    dev::h256 newSeedHash(seedHash.toStdString());

    if(_currentWorkPackage.seedHash != newSeedHash) {
        qDebug() << "Grabbing DAG for" << seedHash;
    }

    if(!(dag = dev::eth::EthashAux::full(newSeedHash, true, [&](unsigned progressCount) {
       emit dagCreationProgress(progressCount);
       emit currentStep(BuildingDAG);
       return 0;
    }))) {
        emit dagCreationFailure();
    }

    if(_configuration.precomputeNextDAG) {
        dev::eth::EthashAux::computeFull(dev::sha3(newSeedHash), true);
    }

    if(newHeaderHash != _currentWorkPackage.headerHash) {
        _currentWorkPackage.headerHash = newHeaderHash;
        _currentWorkPackage.seedHash = newSeedHash;
        _currentWorkPackage.boundary = dev::h256(dev::fromHex(
                                    boundary.toStdString()
                                ), dev::h256::AlignRight);
        _powFarm.setWork(_currentWorkPackage);
    }
}

void EthereumMiner::begin() {
    if(_configuration.minerType == CPUMiner) {
        dev::eth::EthashCPUMiner::setNumInstances(_configuration.maxMiningThreads);
        emit platformInfo(QString::fromStdString(dev::eth::EthashCPUMiner::platformInfo()));
    } else if (_configuration.minerType == OpenCLMiner) {
        if(!dev::eth::EthashGPUMiner::configureGPU(
                    _configuration.localWorkSize,
                    _configuration.globalWorkSizeMultiplier,
                    _configuration.msPerBatch,
                    _configuration.openclPlatform,
                    _configuration.openclDevice,
                    _configuration.recognizeCPUAsOpenCLDevice,
                    _configuration.extraGPUMemory,
                    _configuration.currentBlock
                    )) {
            emit error("Failed to initialize GPU miner.");
        }
        dev::eth::EthashGPUMiner::setNumInstances(_configuration.maxMiningThreads);
        emit platformInfo(QString::fromStdString(dev::eth::EthashGPUMiner::platformInfo()));
    }
    
    std::map<std::string, dev::eth::GenericFarm<dev::eth::EthashProofOfWork>::SealerDescriptor> sealers;
    sealers[sealerString(CPUMiner)] = dev::eth::GenericFarm<dev::eth::EthashProofOfWork>::SealerDescriptor{&dev::eth::EthashCPUMiner::instances, [](dev::eth::GenericMiner<dev::eth::EthashProofOfWork>::ConstructionInfo ci){ return new dev::eth::EthashCPUMiner(ci); }};
    sealers[sealerString(OpenCLMiner)] = dev::eth::GenericFarm<dev::eth::EthashProofOfWork>::SealerDescriptor{&dev::eth::EthashGPUMiner::instances, [](dev::eth::GenericMiner<dev::eth::EthashProofOfWork>::ConstructionInfo ci){ return new dev::eth::EthashGPUMiner(ci); }};

    _powFarm.setSealers(sealers);
    _powFarm.start(sealerString(_configuration.minerType));
    _powFarm.onSolutionFound([&](dev::eth::EthashProofOfWork::Solution sol) {
        QString nonce = "0x" + QString::fromStdString(sol.nonce.hex());
        QString headerHash = "0x" + QString::fromStdString(_currentWorkPackage.headerHash.hex());
        QString mixHash = "0x" + QString::fromStdString(sol.mixHash.hex());

        qDebug() << "Solution found; Submitting ...";
        qDebug() << "  Nonce:" << nonce;
        qDebug() << "  Mixhash:" << mixHash;
        qDebug() << "  Header-hash:" << headerHash;
        qDebug() << "  Seedhash:" << QString::fromStdString(_currentWorkPackage.seedHash.hex());
        qDebug() << "  Target: " << QString::fromStdString(dev::h256(_currentWorkPackage.boundary).hex());
        qDebug() << "  Ethash: " << QString::fromStdString(dev::h256(dev::eth::EthashAux::eval(_currentWorkPackage.seedHash, _currentWorkPackage.headerHash, sol.nonce).value).hex());

        _ethereumProtocol->eth_submitWork(nonce, headerHash, mixHash);
        emit solutionFound(nonce, headerHash, mixHash);
        _currentWorkPackage.reset();
        _ethereumProtocol->eth_getWork();
        return true;
    });

    connectToServer();
}

void EthereumMiner::halt() {
    _powFarm.stop();
}

const char* EthereumMiner::sealerString(MinerType minerType) {
    switch(minerType) {
        case CPUMiner:
            return "cpu";
        case OpenCLMiner:
            return "opencl";
        default:
            return "unknown";
    }
}
