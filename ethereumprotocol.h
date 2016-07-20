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

// Own includes
#include "stratumclient.h"

class EthereumProtocol :
    public QObject {
    Q_OBJECT
public:
    EthereumProtocol(QObject *parent = 0);

    StratumClient *stratumClient();

public slots:
    void eth_login(QString username, QString password);
    void eth_getWork();
    void eth_submitWork(QString nonce, QString headerHash, QString mixHash);

signals:
    void eth_login(bool success);
    void eth_getWork(QString headerHash, QString seedHash, QString boundary);
    void eth_submitWork(bool success);

private slots:
    void translateReply(QString method, QJsonArray result);

private:
    StratumClient _stratumClient;
};
