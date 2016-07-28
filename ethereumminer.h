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

    enum Step {
        Starting,
        Halted,
        Connecting,
        LoggingIn,
        Mining,
        BuildingDAG
    };

    struct Configuration {
        MinerType minerType;
        unsigned openclPlatform;
        unsigned openclDevice;
        unsigned maxMiningThreads;
        uint64_t currentBlock;
        bool recognizeCPUAsOpenCLDevice;
        unsigned extraGPUMemory;
        bool precomputeNextDAG;
        QString username;
        QString password;
        QString server;
        unsigned port;

        unsigned globalWorkSizeMultiplier;
        unsigned localWorkSize;
        unsigned msPerBatch;
    };

    EthereumMiner(QObject *parent = 0);
    ~EthereumMiner();

    QThread *processInBackground();

    void setConfiguration(Configuration configuration) {
        _configuration = configuration;
    }

    Configuration configuration() {
        return _configuration;
    }

    int hashrate();
    void listAvailableGPUs() { dev::eth::EthashGPUMiner::listDevices(); }

    void saveSettings(QSettings *settings);
    void loadSettings(QSettings *settings);

    Step currentStep();

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
    void platformInfo(QString platformInfo);
    void currentStep(EthereumMiner::Step step);

private:
    const char* sealerString(MinerType minerType);
    void setCurrentStep(Step step);

    EthereumProtocol *_ethereumProtocol;

    Configuration _configuration;

    Step _currentStep;

    dev::eth::EthashProofOfWork::WorkPackage _currentWorkPackage;
    dev::eth::GenericFarm<dev::eth::EthashProofOfWork> _powFarm;
    dev::eth::EthashAux::FullType dag;

    int _hashrateMHs;
    QTimer *_hashratePollTimer;

    void beginMining();
};

