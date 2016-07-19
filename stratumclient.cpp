#include "stratumclient.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

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

void StratumClient::eth_login(QString username, QString password) {
    int requestId = incrementRequestCounter();

    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = "eth_login";
    QJsonArray params = { username, password };
    obj["params"] = params;
    obj["id"] = requestId;
    _pendingRequests[requestId] = "eth_login";
    QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    doc.append("\n");
    _tcpSocket->write(doc);
}

void StratumClient::eth_getWork() {
    int requestId = requestCounter++;
    _pendingRequests[requestId] = "eth_getWork";
    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = "eth_getWork";
    QJsonArray params = { };
    obj["params"] = params;
    obj["id"] = requestId;
    QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    doc.append("\n");

    _tcpSocket->write(doc);
}

void StratumClient::eth_submitWork(QString nonce, QString headerHash, QString mixHash) {
    int requestId = incrementRequestCounter();

    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = "eth_submitWork";
    QJsonArray params = { nonce, headerHash, mixHash };
    obj["params"] = params;
    obj["id"] = requestId;
    _pendingRequests[requestId] = "eth_submitWork";

    QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    doc.append("\n");
    _tcpSocket->write(doc);
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

void StratumClient::readyRead() {
    // read the data from the socket
    QJsonDocument document = QJsonDocument::fromJson(_tcpSocket->readAll());
    QJsonObject object = document.object();
    QJsonValue jsonValue = object.value("id");
    int id = jsonValue.toInt();

    QJsonValue result = object.value("result");

    if (result.isBool() && !result.toBool()) {
        qDebug() << "Stratum Server error:" << object.value("error").toString();
        return;
    }

    if (id == 0) {
        qDebug() << "Push: New work package received";
        QJsonArray workData = object["result"].toArray();
        emit eth_getWork(workData[0].toString(), workData[1].toString(), workData[2].toString());
    } else {
        QString method = _pendingRequests[id];
        if (method == "eth_login") {
            _pendingRequests.remove(id);
            qDebug() << "Login to stratum server successful";
            emit eth_login(true);
        }
        if (method == "eth_getWork") {
            _pendingRequests.remove(id);
            qDebug() << "Work package received";
            QJsonArray workData = object["result"].toArray();
            emit eth_getWork(workData[0].toString(), workData[1].toString(), workData[2].toString());
        }
        if (method == "eth_submitWork") {
            _pendingRequests.remove(id);
            qDebug() << "Share submitted to server!";
            emit eth_submitWork(true);
        }
    }
}
