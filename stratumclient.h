#pragma once

#include <QObject>
#include <QTcpSocket>

class StratumClient : public QObject {
    Q_OBJECT
public:
    explicit StratumClient(QObject *parent = 0);

signals:
    void connectedToServer();
    void disconnectedFromServer();

    void eth_login(bool success);
    void eth_getWork(QString headerHash, QString seedHash, QString boundary);
    void eth_submitWork(bool success);

public slots:
    void connectToServer(QString server, uint port);

    void eth_login(QString username, QString password);
    void eth_getWork();
    void eth_submitWork(QString nonce, QString headerHash, QString mixHash);

private slots:
    void connected();
    void disconnected();
    void readyRead();

private:
    void reconnect();

private:
    int incrementRequestCounter();

    QTcpSocket *_tcpSocket;

    int requestCounter = 1;
    QMap<int, QString> _pendingRequests;
};
