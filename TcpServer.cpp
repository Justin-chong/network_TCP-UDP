#include "TcpServer.h"
#include <QDebug>

TcpServerHost::TcpServerHost(QObject *parent)
    : QObject(parent)
{
    m_server = new QTcpServer(this);

    connect(m_server, &QTcpServer::newConnection, this, &TcpServerHost::onNewConnection);
    connect(m_server, &QTcpServer::acceptError, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_server->errorString());
    });
}

TcpServerHost::~TcpServerHost() = default;

bool TcpServerHost::listen(const QString &address, quint16 port)
{
    QHostAddress hostAddr;
    if (address.isEmpty() || address == QLatin1String("0.0.0.0")) {
        hostAddr = QHostAddress::Any;
    } else {
        hostAddr = QHostAddress(address);
    }

    bool ok = m_server->listen(hostAddr, port);
    if (ok) {
        qDebug() << "[TcpServer] Listening on" << hostAddr << ":" << m_server->serverPort();
    } else {
        qWarning() << "[TcpServer] Listen failed:" << m_server->errorString();
        emit errorOccurred(m_server->errorString());
    }
    emit listeningStateChanged(ok);
    return ok;
}

void TcpServerHost::stop()
{
    // 断开所有客户端
    for (auto &client : m_clients) {
        if (client.socket && client.socket->isOpen()) {
            client.socket->disconnectFromHost();
            client.socket->deleteLater();
        }
    }
    m_clients.clear();

    if (m_server->isListening()) {
        m_server->close();
    }
    emit listeningStateChanged(false);
}

qint64 TcpServerHost::broadcast(const QByteArray &data)
{
    if (data.isEmpty()) return -1;

    qint64 totalSent = 0;
    for (auto &client : m_clients) {
        if (client.socket && client.socket->isOpen()) {
            qint64 n = client.socket->write(data);
            if (n != -1) totalSent += n;
        }
    }
    return totalSent;
}

qint64 TcpServerHost::sendToClient(int clientId, const QByteArray &data)
{
    for (const auto &client : m_clients) {
        if (client.id == clientId && client.socket && client.socket->isOpen()) {
            return client.socket->write(data);
        }
    }
    return -1;
}

bool TcpServerHost::isListening() const
{
    return m_server && m_server->isListening();
}

quint16 TcpServerHost::actualPort() const
{
    return m_server ? m_server->serverPort() : 0;
}

QList<QPair<int, QString>> TcpServerHost::connectedClients() const
{
    QList<QPair<int, QString>> result;
    for (const auto &client : m_clients) {
        if (client.socket) {
            result.append(qMakePair(client.id, client.address));
        }
    }
    return result;
}

void TcpServerHost::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        if (!client) break;

        ClientInfo info;
        info.socket = client;
        info.id = m_nextClientId++;
        info.address = client->peerAddress().toString();

        connect(client, &QTcpSocket::disconnected, this, &TcpServerHost::onClientDisconnected);
        connect(client, &QTcpSocket::readyRead, this, &TcpServerHost::onClientDataReceived);

        m_clients.append(info);
        emit clientConnected(info.id, info.address);
        qDebug() << "[TcpServer] Client connected:" << info.address << "ID:" << info.id;
    }
}

void TcpServerHost::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client) return;

    for (int i = 0; i < m_clients.size(); ++i) {
        if (m_clients[i].socket == client) {
            int id = m_clients[i].id;
            QString addr = m_clients[i].address;
            m_clients.removeAt(i);
            emit clientDisconnected(id);
            qDebug() << "[TcpServer] Client disconnected:" << addr << "ID:" << id;
            break;
        }
    }
    client->deleteLater();
}

void TcpServerHost::onClientDataReceived()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client || !client->isOpen()) return;

    // 查找客户端ID
    int clientId = 0;
    for (const auto &c : m_clients) {
        if (c.socket == client) {
            clientId = c.id;
            break;
        }
    }

    QByteArray data = client->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(clientId, data);
    }
}
