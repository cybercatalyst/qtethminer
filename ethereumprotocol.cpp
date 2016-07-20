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

#include "ethereumprotocol.h"

#define ETH_LOGIN "eth_login"
#define ETH_GETWORK "eth_getWork"
#define ETH_SUBMITWORK "eth_submitWork"

EthereumProtocol::EthereumProtocol(QObject *parent) :
    QObject(parent) {
    connect(&_stratumClient, SIGNAL(jsonReplyReceived(QString,QJsonArray)),
            this, SLOT(translateReply(QString,QJsonArray)));
}

StratumClient *EthereumProtocol::stratumClient() {
    return &_stratumClient;
}

void EthereumProtocol::eth_login(QString username, QString password) {
    _stratumClient.sendJsonRpc(ETH_LOGIN, { username, password });
}

void EthereumProtocol::eth_getWork() {
    _stratumClient.sendJsonRpc(ETH_GETWORK, { });
}

void EthereumProtocol::eth_submitWork(QString nonce, QString headerHash, QString mixHash) {
    _stratumClient.sendJsonRpc(ETH_SUBMITWORK, { nonce, headerHash, mixHash });
}

void EthereumProtocol::translateReply(QString method, QJsonArray result) {
    if (method == "" || method == ETH_GETWORK) {
        qDebug() << "Push: New work package received";
        emit eth_getWork(result[0].toString(), result[1].toString(), result[2].toString());
    }

    if (method == ETH_LOGIN) {
        qDebug() << "Login to stratum server successful";
        emit eth_login(true);
    }

    if (method == ETH_SUBMITWORK) {
        qDebug() << "Share submitted to server!";
        emit eth_submitWork(true);
    }
}
