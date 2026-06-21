#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QList>
#include <QString>

/**
 * ��@brief TCP 服务端封装
 *
 * 封装 QTcpServer，管理多个客户端连接。
 * 不暴露原始 QTcpSocket 指针，通过客户端ID进行通信。
 */
class TcpServerHost : public QObject
{
    Q_OBJECT

public:
    explicit TcpServerHost(QObject *parent = nullptr);
    ~TcpServerHost() override;

    // 监听/停止
    bool listen(const QString &address, quint16 port);
    void stop();

    // 发送数据
    qint64 broadcast(const QByteArray &data);               // 广播给所有客户端
    qint64 sendToClient(int clientId, const QByteArray &data); // 发送给指定客户端

    // 状态查询
    bool isListening() const;
    quint16 actualPort() const;
    QList<QPair<int, QString>> connectedClients() const;   // id, address

signals:
    void clientConnected(int clientId, const QString &address);
    void clientDisconnected(int clientId);
    void dataReceived(int clientId, const QByteArray &data);
    void listeningStateChanged(bool listening);
    void errorOccurred(const QString &errorString);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onClientDataReceived();

private:
    QTcpServer *m_server = nullptr;
    struct ClientInfo {
        QTcpSocket *socket = nullptr;
        int id = 0;
        QString address;
    };
    QList<ClientInfo> m_clients;
    int m_nextClientId = 1;
};

#endif // TCPSERVER_H
