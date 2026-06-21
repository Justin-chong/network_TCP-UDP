#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>

/**
 * @brief UDP 通信封装
 *
 * 支持单播、广播通信模式。
 * （组播功能已暂时禁用）
 */
class UdpSocket : public QObject
{
    Q_OBJECT

public:
    enum class Mode { Unicast, Broadcast /*, Multicast*/ };
    Q_ENUM(Mode)

    explicit UdpSocket(QObject *parent = nullptr);
    ~UdpSocket() override;

    // 绑定端口
    bool bind(quint16 port);
    void close();

    // 发送数据
    qint64 sendTo(const QString &address, quint16 port, const QByteArray &data);
    qint64 sendBroadcast(quint16 port, const QByteArray &data);
    // qint64 sendMulticast(const QString &groupAddress, quint16 port, const QByteArray &data);  // 暂时禁用

    // 组播设置（暂时禁用）
    // bool joinMulticastGroup(const QString &groupAddress);
    // bool leaveMulticastGroup(const QString &groupAddress);

    // 状态查询
    bool isBound() const;
    quint16 boundPort() const;

    // 设置发送模式（用于广播/组播）
    void setMode(Mode mode) { m_mode = mode; }
    Mode mode() const { return m_mode; }

signals:
    void dataReceived(const QByteArray &data, const QString &senderAddress, quint16 senderPort);
    void bindingStateChanged(bool bound);
    void errorOccurred(const QString &errorString);

private slots:
    void onReadyRead();

private:
    QUdpSocket *m_udpSocket = nullptr;
    Mode m_mode = Mode::Unicast;
};

#endif // UDPSOCKET_H
