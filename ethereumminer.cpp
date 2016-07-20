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

EthereumMiner::EthereumMiner() :
    QThread(0) {
    moveToThread(this);
}

EthereumMiner::~EthereumMiner() {
}

void EthereumMiner::startMining() {
    start();
}

void EthereumMiner::stopMining() {
    quit();
}

void EthereumMiner::restartMining() {
    quit();
    start();
}

void EthereumMiner::run() {
    _hashrateReportTimer = new QTimer(this);
    _hashrateReportTimer->setInterval(500);

    connect(_hashrateReportTimer, SIGNAL(timeout()), this, SLOT(updateHashrate()));

    connect(_ethereumProtocol.stratumClient(), SIGNAL(disconnectedFromServer()), this, SLOT(connectToServer()));
    connect(_ethereumProtocol.stratumClient(), SIGNAL(connectedToServer()), this, SLOT(login()));
    connect(&_ethereumProtocol, SIGNAL(eth_login(bool)), &_ethereumProtocol, SLOT(eth_getWork()));
    connect(&_ethereumProtocol, SIGNAL(eth_getWork(QString,QString,QString)), this, SLOT(processWorkPackage(QString,QString,QString)));

    connectToServer();

    _hashrateReportTimer->start();
    prepareMiner();
    exec();
}

void EthereumMiner::connectToServer() {
    _ethereumProtocol.stratumClient()->connectToServer(_configuration.m_server, _configuration.m_port);
}

void EthereumMiner::login() {
    _ethereumProtocol.eth_login(_configuration.m_user, _configuration.m_pass);
}

void EthereumMiner::updateHashrate() {
    dev::eth::WorkingProgress wp = _powFarm.miningProgress();
    _powFarm.resetMiningProgress();
    qDebug() << "Mining at" << (int)wp.rate() << "H/s";
    emit hashrate((int)wp.rate());
}

void EthereumMiner::processWorkPackage(QString headerHash, QString seedHash, QString boundary) {
    qDebug() << "Received work package.";
    emit receivedWorkPackage(headerHash, seedHash, boundary);

    dev::h256 newHeaderHash(headerHash.toStdString());
    dev::h256 newSeedHash(seedHash.toStdString());

    if(_currentWorkPackage.seedHash != newSeedHash) {
        qDebug() << "Grabbing DAG for" << seedHash;
    }

    if(!(dag = dev::eth::EthashAux::full(newSeedHash, true, [&](unsigned progressCount) {
       emit dagCreationProgress(progressCount);
       return 0;
    }))) {
        emit dagCreationFailure();
    }

    if(_configuration._precomputeDAG) {
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

void EthereumMiner::prepareMiner() {
    if(_configuration._minerType == "cpu") {
        dev::eth::EthashCPUMiner::setNumInstances(_configuration.m_miningThreads);
    } else if (_configuration._minerType == "opencl") {
        if(!dev::eth::EthashGPUMiner::configureGPU(
                    _configuration._localWorkSize,
                    _configuration._globalWorkSizeMultiplier,
                    _configuration._msPerBatch,
                    _configuration._openclPlatform,
                    _configuration._openclDevice,
                    _configuration._clAllowCPU,
                    _configuration._extraGPUMemory,
                    _configuration._currentBlock
                    )) {
            emit error("Failed to initialize GPU miner.");
        }
        dev::eth::EthashGPUMiner::setNumInstances(_configuration.m_miningThreads);
    }
    
    std::map<std::string, dev::eth::GenericFarm<dev::eth::EthashProofOfWork>::SealerDescriptor> sealers;
    sealers["cpu"] = dev::eth::GenericFarm<dev::eth::EthashProofOfWork>::SealerDescriptor{&dev::eth::EthashCPUMiner::instances, [](dev::eth::GenericMiner<dev::eth::EthashProofOfWork>::ConstructionInfo ci){ return new dev::eth::EthashCPUMiner(ci); }};
    sealers["opencl"] = dev::eth::GenericFarm<dev::eth::EthashProofOfWork>::SealerDescriptor{&dev::eth::EthashGPUMiner::instances, [](dev::eth::GenericMiner<dev::eth::EthashProofOfWork>::ConstructionInfo ci){ return new dev::eth::EthashGPUMiner(ci); }};

    _powFarm.setSealers(sealers);
    _powFarm.start(_configuration._minerType);
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

        _ethereumProtocol.eth_submitWork(nonce, headerHash, mixHash);
        emit solutionFound(nonce, headerHash, mixHash);
        _currentWorkPackage.reset();
        _ethereumProtocol.eth_getWork();
        return true;
    });
}


















