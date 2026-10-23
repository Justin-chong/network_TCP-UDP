#include "UDP.h"
#include <QDebug>

UdpSocket::UdpSocket(QObject *parent)
    : QObject(parent)
{
    m_udpSocket = new QUdpSocket(this);

    connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpSocket::onReadyRead);
    connect(m_udpSocket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::errorOccurred),
            this, [this](QAbstractSocket::SocketError error) {
                Q_UNUSED(error);
                emit errorOccurred(m_udpSocket->errorString());
            });
}

UdpSocket::~UdpSocket() = default;

bool UdpSocket::bind(quint16 port)
{
    if (m_udpSocket->isOpen()) {
        m_udpSocket->close();
    }

    bool result = m_udpSocket->bind(port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    emit bindingStateChanged(result);
    if (result) {
        qDebug() << "[UDP] Bound to port:" << m_udpSocket->localPort();
    }
    return result;
}

void UdpSocket::close()
{
    m_udpSocket->close();
    emit bindingStateChanged(false);
}

qint64 UdpSocket::sendTo(const QString &address, quint16 port, const QByteArray &data)
{
    QHostAddress hostAddr(address);
    return m_udpSocket->writeDatagram(data, hostAddr, port);
}

qint64 UdpSocket::sendBroadcast(quint16 port, const QByteArray &data)
{
    return m_udpSocket->writeDatagram(data, QHostAddress::Broadcast, port);
}

bool UdpSocket::isBound() const
{
    return m_udpSocket && m_udpSocket->state() == QAbstractSocket::BoundState;
}

quint16 UdpSocket::boundPort() const
{
    return m_udpSocket ? m_udpSocket->localPort() : 0;
}

void UdpSocket::onReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        qint64 size = m_udpSocket->pendingDatagramSize();
        QByteArray data(size, 0);

        QHostAddress senderAddr;
        quint16 senderPort = 0;

        m_udpSocket->readDatagram(data.data(), size, &senderAddr, &senderPort);
        emit dataReceived(data, senderAddr.toString(), senderPort);
    }
}
