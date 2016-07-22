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

// Qt includes
#include <QObject>
#include <QTcpSocket>
#include <QJsonArray>

class StratumClient :
    public QObject {
    Q_OBJECT
public:
    explicit StratumClient(QObject *parent = 0);

    void sendJsonRpc(QString method, QJsonArray parameters);

signals:
    void connectedToServer();
    void disconnectedFromServer();

    void jsonReplyReceived(QString method, QJsonArray result);

public slots:
    void connectToServer(QString server, uint port);

private slots:
    void connected();
    void disconnected();
    void readyRead();

private:
    int incrementRequestCounter();

    QTcpSocket *_tcpSocket;

    int requestCounter = 1;
    // Theoretically, it could be possible that failing requests
    // may never get wiped out of this map. This is not urgent,
    // but should be checked.
    QMap<int, QString> _pendingRequests;
};
