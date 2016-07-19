#pragma once

#define ETH_ETHASHCL 1
#include "ethashseal/EthashGPUMiner.h"
#include "ethashseal/EthashCPUMiner.h"
#include "ethash-cl/ethash_cl_miner.h"

#include <QTimer>
#include <QThread>

#include <stdint.h>

#include "stratumclient.h"

class EthereumMiner: public QThread {
	Q_OBJECT

public:
    EthereumMiner();
    ~EthereumMiner();
  
    void enableCPUMining() { _minerType = "cpu"; }
    void enableGPUMining() { _minerType = "opencl"; }
    void setMiningThreads(unsigned miningThreads) { m_miningThreads = miningThreads; }
    void setGlobalWorkSizeMultiplier(unsigned globalWorkSizeMultiplier) { _globalWorkSizeMultiplier = globalWorkSizeMultiplier; }
    void setLocalWorkSize(unsigned localWorkSize) { _localWorkSize = localWorkSize; }
    void setMsPerBatch(unsigned msPerBatch) { _msPerBatch = msPerBatch; }
    void setExtraGPUMemory(unsigned extraGPUMemory) { _extraGPUMemory = extraGPUMemory; }
    void setOpenclDevice(unsigned openclDevice) { _openclDevice = openclDevice; m_miningThreads = 1; }
    void setOpenclPlatform(unsigned openclPlatform) { _openclPlatform = openclPlatform; }
    void setCurrentBlock(uint64_t currentBlock) { _currentBlock = currentBlock; }
    void setUser(QString user) { m_user = user; }
    void setPass(QString pass) { m_pass = pass; }
    void setServer(QString server) { m_server = server; }
    void setPort(unsigned port) { m_port = port; }
    void disablePrecompute() { _precomputeDAG = false; }
    void listDevices() { dev::eth::EthashGPUMiner::listDevices(); emit finished(); }

public slots:
    void run();

private slots:
    void connectToServer();
    void login();
    void requestWorkPackage();
    void processWorkPackage(QString headerHash, QString seedHash, QString boundary);

    void updateHashrate();

signals:
    void hashrate(int hashrate);
    void finished();
    void dagCreationProgress(int progressCount);
    void receivedWorkPackage(QString headerHash, QString seedHash, QString boundary);
    void solutionFound(QString nonce, QString headerHash, QString mixHash);
    void dagCreationFailure();

private:
    StratumClient *_stratumClient;

    std::string _minerType = "cpu";
    unsigned _openclPlatform = 0;
    unsigned _openclDevice = 0;
    unsigned m_miningThreads = UINT_MAX;
    uint64_t _currentBlock = 0;
    bool _clAllowCPU = false;
    unsigned _extraGPUMemory = 64000000;
    bool _precomputeDAG = true;
    QString m_user = "";
    QString m_pass = "x";
    QString m_server = "ethpool.org";
    unsigned m_port = 3333;

    unsigned _globalWorkSizeMultiplier = ethash_cl_miner::c_defaultGlobalWorkSizeMultiplier;
    unsigned _localWorkSize = ethash_cl_miner::c_defaultLocalWorkSize;
    unsigned _msPerBatch = ethash_cl_miner::c_defaultMSPerBatch;

    dev::eth::EthashProofOfWork::WorkPackage _currentWorkPackage;
    dev::eth::GenericFarm<dev::eth::EthashProofOfWork> _powFarm;
    dev::eth::EthashAux::FullType dag;

    QTimer *_hashrateReportTimer;

    void prepareMiner();
};

