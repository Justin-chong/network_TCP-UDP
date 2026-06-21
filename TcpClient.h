#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

/**
 * @brief TCP 客户端连接封装
 *
 * 封装 QTcpSocket，提供连接管理、数据收发、状态查询功能。
 * 通过信号通知外部连接状态变化和数据到达事件。
 */
class TcpConnection : public QObject
{
    Q_OBJECT

public:
    explicit TcpConnection(QObject *parent = nullptr);
    ~TcpConnection() override;

    // 连接操作
    void connectToHost(const QString &host, quint16 port);
    void disconnectFromHost();

    // 发送数据
    qint64 sendData(const QByteArray &data);

    // 状态查询
    bool isConnected() const;
    QString peerAddress() const;
    quint16 peerPort() const;

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &errorString);

private slots:
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QTcpSocket *m_socket = nullptr;
};

#endif // TCPCONNECTION_H
