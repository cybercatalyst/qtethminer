#include "ethereumminer.h"

#include <QDebug>

using namespace std;
using namespace dev;
using namespace dev::eth;

EthereumMiner::EthereumMiner() :
    QThread(0) {
    moveToThread(this);
}

EthereumMiner::~EthereumMiner() {
}

void EthereumMiner::run() {
    _stratumClient = new StratumClient(this);

    _hashrateReportTimer = new QTimer(this);
    _hashrateReportTimer->setInterval(2000);

    connect(_hashrateReportTimer, SIGNAL(timeout()), this, SLOT(updateHashrate()));

    connect(_stratumClient, SIGNAL(disconnectedFromServer()), this, SLOT(connectToServer()));
    connect(_stratumClient, SIGNAL(connectedToServer()), this, SLOT(login()));
    connect(_stratumClient, SIGNAL(eth_login(bool)), this, SLOT(requestWorkPackage()));
    connect(_stratumClient, SIGNAL(eth_getWork(QString,QString,QString)), this, SLOT(processWorkPackage(QString,QString,QString)));

    connectToServer();

    _hashrateReportTimer->start();
    prepareMiner();
    exec();
}

void EthereumMiner::connectToServer() {
    _stratumClient->connectToServer(m_server, m_port);
}

void EthereumMiner::login() {
    _stratumClient->eth_login(m_user, m_pass);
}

void EthereumMiner::requestWorkPackage() {
    _stratumClient->eth_getWork();
}

void EthereumMiner::updateHashrate() {
    WorkingProgress wp = _powFarm.miningProgress();
    _powFarm.resetMiningProgress();
    qDebug() << "Mining at" << (int)wp.rate() << "H/s";
    emit hashrate((int)wp.rate());
}

void EthereumMiner::processWorkPackage(QString headerHash, QString seedHash, QString boundary) {
    qDebug() << "Received work package.";
    emit receivedWorkPackage(headerHash, seedHash, boundary);

    h256 newHeaderHash(headerHash.toStdString());
    h256 newSeedHash(seedHash.toStdString());

    if(_currentWorkPackage.seedHash != newSeedHash) {
        qDebug() << "Grabbing DAG for" << seedHash;
    }

    if(!(dag = EthashAux::full(newSeedHash, true, [&](unsigned progressCount) {
                               emit dagCreationProgress(progressCount);
                               return 0;
}))) {
        emit dagCreationFailure();
    }

    if(_precomputeDAG) {
        EthashAux::computeFull(sha3(newSeedHash), true);
    }

    if(newHeaderHash != _currentWorkPackage.headerHash) {
        _currentWorkPackage.headerHash = newHeaderHash;
        _currentWorkPackage.seedHash = newSeedHash;
        _currentWorkPackage.boundary = h256(fromHex(
                                    boundary.toStdString()
                                ), h256::AlignRight);
        _powFarm.setWork(_currentWorkPackage);
    }
}

void EthereumMiner::prepareMiner() {
    if(_minerType == "cpu") {
        EthashCPUMiner::setNumInstances(m_miningThreads);
    } else if (_minerType == "opencl") {
        if(!EthashGPUMiner::configureGPU(
                    _localWorkSize,
                    _globalWorkSizeMultiplier,
                    _msPerBatch,
                    _openclPlatform,
                    _openclDevice,
                    _clAllowCPU,
                    _extraGPUMemory,
                    _currentBlock
                    ))
            exit(1);
        EthashGPUMiner::setNumInstances(m_miningThreads);
    }
    
    map<string, GenericFarm<EthashProofOfWork>::SealerDescriptor> sealers;
    sealers["cpu"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashCPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashCPUMiner(ci); }};
    sealers["opencl"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashGPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashGPUMiner(ci); }};

    _powFarm.setSealers(sealers);
    _powFarm.start(_minerType);
    _powFarm.onSolutionFound([&](EthashProofOfWork::Solution sol) {
        QString nonce = "0x" + QString::fromStdString(sol.nonce.hex());
        QString headerHash = "0x" + QString::fromStdString(_currentWorkPackage.headerHash.hex());
        QString mixHash = "0x" + QString::fromStdString(sol.mixHash.hex());

        qDebug() << "Solution found; Submitting ...";
        qDebug() << "  Nonce:" << nonce;
        qDebug() << "  Mixhash:" << mixHash;
        qDebug() << "  Header-hash:" << headerHash;
        qDebug() << "  Seedhash:" << QString::fromStdString(_currentWorkPackage.seedHash.hex());
        qDebug() << "  Target: " << QString::fromStdString(h256(_currentWorkPackage.boundary).hex());
        qDebug() << "  Ethash: " << QString::fromStdString(h256(EthashAux::eval(_currentWorkPackage.seedHash, _currentWorkPackage.headerHash, sol.nonce).value).hex());


        _stratumClient->eth_submitWork(nonce, headerHash, mixHash);
        emit solutionFound(nonce, headerHash, mixHash);
        _currentWorkPackage.reset();
        _stratumClient->eth_getWork();
        return true;
    });
}


















