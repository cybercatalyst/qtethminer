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

#pragma once

#define ETH_ETHASHCL 1
#include "ethashseal/EthashGPUMiner.h"
#include "ethashseal/EthashCPUMiner.h"
#include "ethash-cl/ethash_cl_miner.h"

#include <QTimer>
#include <QThread>
#include <QSettings>

#include <stdint.h>

#include "ethereumprotocol.h"

class EthereumMiner :
    public QObject {
	Q_OBJECT

public:
    enum MinerType {
        CPUMiner,
        OpenCLMiner
    };

    struct MiningConfiguration {
        MinerType minerType;
        unsigned openclPlatform = 0;
        unsigned openclDevice = 0;
        unsigned maxMiningThreads = UINT_MAX;
        uint64_t currentBlock = 0;
        bool recognizeCPUAsOpenCLDevice = false;
        unsigned extraGPUMemory = 64000000;
        bool precomputeNextDAG = true;
        QString username = "";
        QString password = "x";
        QString server = "ethpool.org";
        unsigned port = 3333;

        unsigned globalWorkSizeMultiplier = ethash_cl_miner::c_defaultGlobalWorkSizeMultiplier;
        unsigned localWorkSize = ethash_cl_miner::c_defaultLocalWorkSize;
        unsigned msPerBatch = ethash_cl_miner::c_defaultMSPerBatch;
    };

    EthereumMiner();
    ~EthereumMiner();

    void setMiningConfiguration(MiningConfiguration miningConfiguration) {
        _configuration = miningConfiguration;
    }

    MiningConfiguration miningConfiguration() {
        return _configuration;
    }

    int hashrate();
    void listAvailableGPUs() { dev::eth::EthashGPUMiner::listDevices(); }

    void saveSettings(QSettings& settings);
    void loadSettings(QSettings& settings);

public slots:
    void startMining();
    void stopMining();
    void restartMining();

private slots:
    void begin();
    void halt();
    void handleDisconnect();
    void connectToServer();
    void login();
    void processWorkPackage(QString headerHash, QString seedHash, QString boundary);

signals:
    void hashrate(int hashrate);
    void dagCreationProgress(int progressCount);
    void receivedWorkPackage(QString headerHash, QString seedHash, QString boundary);
    void solutionFound(QString nonce, QString headerHash, QString mixHash);
    void dagCreationFailure();
    void error(QString message);

private:
    const char* sealerString(MinerType minerType);

    EthereumProtocol *_ethereumProtocol;

    MiningConfiguration _configuration;

    dev::eth::EthashProofOfWork::WorkPackage _currentWorkPackage;
    dev::eth::GenericFarm<dev::eth::EthashProofOfWork> _powFarm;
    dev::eth::EthashAux::FullType dag;

    int _hashrateMHs;
    QTimer *_hashratePollTimer;

    void beginMining();
};

