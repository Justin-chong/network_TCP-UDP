#include "TcpClient.h"
#include <QDebug>

TcpConnection::TcpConnection(QObject *parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &TcpConnection::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpConnection::disconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpConnection::onErrorOccurred);
}

TcpConnection::~TcpConnection() = default;

void TcpConnection::connectToHost(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void TcpConnection::disconnectFromHost()
{
    if (m_socket->isOpen()) {
        m_socket->disconnectFromHost();
    }
}

qint64 TcpConnection::sendData(const QByteArray &data)
{
    if (!isConnected()) {
        qWarning() << "Not connected, cannot send data";
        return 0;
    }
    qint64 bytes = m_socket->write(data);
    if (bytes == -1) {
        qWarning() << "Send failed:" << m_socket->errorString();
    }
    return bytes;
}

bool TcpConnection::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

QString TcpConnection::peerAddress() const
{
    return m_socket ? m_socket->peerAddress().toString() : QString();
}

quint16 TcpConnection::peerPort() const
{
    return m_socket ? m_socket->peerPort() : 0;
}

void TcpConnection::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void TcpConnection::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    emit errorOccurred(m_socket->errorString());
}
