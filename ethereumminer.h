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

#include <stdint.h>

#include "ethereumprotocol.h"

class EthereumMiner: public QThread {
	Q_OBJECT

public:
    enum MiningStatus {
        Idle,
        Connecting,
        LoggingIn,
        WaitingForWork,
        Mining,
        SubmittingWork
    };

    struct MiningConfiguration {
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
    };

    EthereumMiner();
    ~EthereumMiner();

    void setMiningConfiguration(MiningConfiguration miningConfiguration) {
        _configuration = miningConfiguration;
    }

    MiningConfiguration miningConfiguration() {
        return _configuration;
    }

    void listAvailableGPUs() { dev::eth::EthashGPUMiner::listDevices(); }

public slots:
    void run();

private slots:
    void connectToServer();
    void login();
    void processWorkPackage(QString headerHash, QString seedHash, QString boundary);

    void updateHashrate();

signals:
    void hashrate(int hashrate);
    void dagCreationProgress(int progressCount);
    void receivedWorkPackage(QString headerHash, QString seedHash, QString boundary);
    void solutionFound(QString nonce, QString headerHash, QString mixHash);
    void dagCreationFailure();
    void error(QString message);

private:
    MiningStatus _miningStatus;
    EthereumProtocol _ethereumProtocol;

    MiningConfiguration _configuration;

    dev::eth::EthashProofOfWork::WorkPackage _currentWorkPackage;
    dev::eth::GenericFarm<dev::eth::EthashProofOfWork> _powFarm;
    dev::eth::EthashAux::FullType dag;

    QTimer *_hashrateReportTimer;

    void prepareMiner();
};

