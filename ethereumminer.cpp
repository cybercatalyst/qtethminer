#include "ethereumminer.h"

#include <QDebug>

using namespace std;
using namespace dev;
using namespace dev::eth;

EthereumMiner::EthereumMiner() :
    QThread(0) {
    connect(this, SIGNAL(solutionFound(QString, QString, QString)),
            this, SLOT(submitWork(QString, QString, QString)));
}

EthereumMiner::~EthereumMiner() {
}

void EthereumMiner::run() {
    _hashrateReportTimer = new QTimer();
    _hashrateReportTimer->setInterval(2000);
    _serverPingTimer = new QTimer();
    _serverPingTimer->setInterval(30000);
    _serverPingTimer->setSingleShot(true);

    connect(_hashrateReportTimer, SIGNAL(timeout()), this, SLOT(updateHashrate()));
    connect(_serverPingTimer, SIGNAL(timeout()), this, SLOT(pingServer()));

    //qDebug() << "Connecting to stratum server " << m_server << ":" << m_port;

    _tcpSocket = new QTcpSocket();
    connect(_tcpSocket, SIGNAL(connected()),
            this, SLOT(connected()));
    connect(_tcpSocket, SIGNAL(disconnected()),
            this, SLOT(disconnected()));
    connect(_tcpSocket, SIGNAL(readyRead()),
            this, SLOT(readyRead()));

    reconnect();
    _hashrateReportTimer->start();
    _serverPingTimer->start();
    initializeMining();
}

void EthereumMiner::sendGetWorkRequest() {
    int requestId = requestCounter++;
    // qDebug() << requestId;
    requests[requestId] = "eth_getWork";
    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = "eth_getWork";
    QJsonArray params = { };
    obj["params"] = params;
    obj["id"] = requestId;
    QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    doc.append("\n");

    //qDebug() << doc;
    _tcpSocket->write(doc);
}

void EthereumMiner::connected() {
    qDebug() << "Connection to stratum server established!";
    
    int requestId = requestCounter++;
    
    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = "eth_login";
    QJsonArray params = { m_user, m_pass };
    obj["params"] = params;
    obj["id"] = requestId;
    requests[requestId] = "eth_login";
    QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    doc.append("\n");
    
    // qDebug() << doc;
    
    _tcpSocket->write(doc);
}

void EthereumMiner::disconnected() {
    _hashrateReportTimer->stop();
    _serverPingTimer->stop();
    qDebug() << "Connection to stratum server lost. Trying to reconnect!";
    reconnect();
}

void EthereumMiner::reconnect() {
    while (true) {
        qDebug() << "(Re)connecting..";
        _tcpSocket->connectToHost(m_server, m_port);
        if(_tcpSocket->waitForConnected(5000)) {
            return;
        } else {
            qDebug() << "Stratum connection error: " << _tcpSocket->errorString();
        }
    }
}

void EthereumMiner::readyRead() {
    //qDebug() << "reading...";

    // read the data from the socket
    QByteArray jsonData = _tcpSocket->readAll();
    // qDebug() << jsonData;
    QJsonDocument document = QJsonDocument::fromJson(jsonData);
    QJsonObject object = document.object();
    QJsonValue jsonValue = object.value("id");
    double id = jsonValue.toDouble();

    // qDebug() << "Id is: " << id;
    QJsonValue result = object.value("result");

    if (result.isBool() && !result.toBool()) {
        qDebug() << "Stratum Server error:" << object.value("error").toString();
        sendGetWorkRequest();
        return;
    }

    if (id == 0) {
        qDebug() << "Push: New work package received";
        QJsonArray workData = object["result"].toArray();
        processWorkPackage(workData);
    } else {
        QString method = requests[id];
        // qDebug() << method;
        if (method == "eth_login") {
            requests.erase(id);
            qDebug() << "Login to stratum server successfull";
            sendGetWorkRequest();
        }
        if (method == "eth_getWork") {
            requests.erase(id);
            qDebug() << "Work package received";
            QJsonArray workData = object["result"].toArray();
            processWorkPackage(workData);
        }
        if (method == "eth_submitWork") {
            requests.erase(id);
            qDebug() << "Share submitted to server!";
            sendGetWorkRequest();
        }
    }
}

void EthereumMiner::updateHashrate() {
    qDebug() << "Update hashrate";
    WorkingProgress wp = _powFarm.miningProgress();
    _powFarm.resetMiningProgress();
    emit hashrate((int)wp.rate());
}

void EthereumMiner::pingServer() {
    qDebug() << "Sending getwork";
    sendGetWorkRequest();
}

void EthereumMiner::processWorkPackage(QJsonArray workData) {
    h256 headerHash(workData[0].toString().toStdString());
    h256 newSeedHash(workData[1].toString().toStdString());

    // qDebug() << current.seedHash;
    // cout << newSeedHash << endl;
    if(current.seedHash != newSeedHash) {
        qDebug() << "Grabbing DAG for";// << newSeedHash;
    }

    if(!(dag = EthashAux::full(newSeedHash, true, [&](unsigned _pc) {
                               emit dagCreationProgress(_pc);
                               return 0;
}))) {
        BOOST_THROW_EXCEPTION(DAGCreationFailure());
    }

    if(m_precompute) {
        EthashAux::computeFull(sha3(newSeedHash), true);
    }

    if(headerHash != current.headerHash) {
        current.headerHash = headerHash;
        current.seedHash = newSeedHash;
        current.boundary = h256(fromHex(
                                    workData[2].toString().toStdString()
                                ), h256::AlignRight);

        emit receivedWorkPackage(
                    QString::fromStdString(current.headerHash.hex()),
                    QString::fromStdString(current.seedHash.hex()),
                    QString::fromStdString(h256(current.boundary).hex())
                    );

        _powFarm.setWork(current);
    }
}

void EthereumMiner::initializeMining() {
    if(_minerType == "cpu") {
        EthashCPUMiner::setNumInstances(m_miningThreads);
    } else if (_minerType == "opencl") {
        if(!EthashGPUMiner::configureGPU(
                    _localWorkSize,
                    _globalWorkSizeMultiplier,
                    _msPerBatch,
                    _openclPlatform,
                    _openclDevice,
                    _clAllowCPU,
                    _extraGPUMemory,
                    _currentBlock
                    ))
            exit(1);
        EthashGPUMiner::setNumInstances(m_miningThreads);
    }
    
    map<string, GenericFarm<EthashProofOfWork>::SealerDescriptor> sealers;
    sealers["cpu"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashCPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashCPUMiner(ci); }};
    sealers["opencl"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashGPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashGPUMiner(ci); }};

    _powFarm.setSealers(sealers);
    _powFarm.start(_minerType);
    _powFarm.onSolutionFound([&](EthashProofOfWork::Solution sol) {
        qDebug() << "Solution found; Submitting ...";
        qDebug() << "  Nonce:" << QString::fromStdString(sol.nonce.hex());
        qDebug() << "  Mixhash:" << QString::fromStdString(sol.mixHash.hex());
        qDebug() << "  Header-hash:" << QString::fromStdString(current.headerHash.hex());
        qDebug() << "  Seedhash:" << QString::fromStdString(current.seedHash.hex());
        qDebug() << "  Target: " << QString::fromStdString(h256(current.boundary).hex());
        qDebug() << "  Ethash: " << QString::fromStdString(h256(EthashAux::eval(current.seedHash, current.headerHash, sol.nonce).value).hex());
        emit solutionFound("0x" + QString::fromStdString(sol.nonce.hex()), "0x" + QString::fromStdString(current.headerHash.hex()), "0x" + QString::fromStdString(sol.mixHash.hex()));
        return true;
    });
}

void EthereumMiner::submitWork(QString nonce, QString headerHash, QString mixHash) {
    int requestId = requestCounter++;
    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = "eth_submitWork";
    QJsonArray params = { nonce, headerHash, mixHash };
    obj["params"] = params;
    obj["id"] = requestId;
    requests[requestId] = "eth_submitWork";
    QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    doc.append("\n");

    _tcpSocket->write(doc);
    current.reset();
}

















