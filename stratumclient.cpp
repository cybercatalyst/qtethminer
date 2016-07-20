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

// Qt includes
#include <QJsonObject>
#include <QJsonDocument>

// Own includes
#include "stratumclient.h"

StratumClient::StratumClient(QObject *parent) : QObject(parent) {
    _tcpSocket = new QTcpSocket(this);
    connect(_tcpSocket, SIGNAL(connected()),
            this, SLOT(connected()));
    connect(_tcpSocket, SIGNAL(disconnected()),
            this, SLOT(disconnected()));
    connect(_tcpSocket, SIGNAL(readyRead()),
            this, SLOT(readyRead()));
}

void StratumClient::connectToServer(QString server, uint port) {
    qDebug() << "Connecting to stratum server "
             << server << ":" << port;
    _tcpSocket->connectToHost(server, port);
}

void StratumClient::connected() {
    qDebug() << "Connection to stratum server established.";
    emit connectedToServer();
}

void StratumClient::disconnected() {
    qDebug() << "Connection to stratum server lost.";
    emit disconnectedFromServer();
}

int StratumClient::incrementRequestCounter() {
    // TODO: Make this thread safe.
    return (requestCounter = requestCounter++);
}

void StratumClient::sendJsonRpc(QString method, QJsonArray parameters) {
    int requestId = incrementRequestCounter();
    _pendingRequests[requestId] = method;

    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = method;
    obj["params"] = parameters;
    obj["id"] = requestId;
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    data.append("\n");
    _tcpSocket->write(data);
}

void StratumClient::readyRead() {
    QJsonObject object = QJsonDocument::fromJson(_tcpSocket->readAll())
        .object();

    int id = object.value("id").toInt();
    QJsonValue result = object.value("result");

    if(result.isBool() && !result.toBool()) {
        qDebug() << "Stratum Server error:"
                 << object.value("error").toString();
        return;
    }

    QString method = _pendingRequests.value(id, "");
    emit jsonReplyReceived(method, result.toArray());

    _pendingRequests.remove(id);
}
