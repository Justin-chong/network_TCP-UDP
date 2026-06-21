#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// Forward declarations
class TcpConnection;
class TcpServerHost;
class UdpSocket;
class SendModel;
class ProtocalParser;

/**
 * @brief TCP/UDP 调试助手主窗口
 *
 * 支持 TCP Client、TCP Server、UDP 三种模式，
 * 提供 ASCII/Hex 收发、文件发送、接收保存等功能。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // 协议类型切换
    void onProtocolTypeChanged(int index);

    // 连接控制
    void onConnectClicked();

    // 发送控制
    void onSendClicked();
    void onAutoSendTimeout();


    // TCP Client 信号
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpDataReceived(const QByteArray &data);
    void onTcpError(const QString &error);

    // TCP Server 信号
    void onServerClientConnected(int clientId, const QString &address);
    void onServerClientDisconnected(int clientId);
    void onServerDataReceived(int clientId, const QByteArray &data);
    void onServerListeningStateChanged(bool listening);
    void onServerError(const QString &error);

    // UDP 信号
    void onUdpDataReceived(const QByteArray &data, const QString &senderAddress, quint16 senderPort);
    void onUdpBindingStateChanged(bool bound);
    void onUdpError(const QString &error);

    // UI 控制
    void onSendModeChanged(bool checked);
    void onBrowseFileClicked();
    void onClearReceiveClicked();
    void onClearSendClicked();
    void onSaveToFileToggled(bool checked);

    // 协议解析
    void onEnableProtocolToggled(bool checked);

private:
    enum class Protocol { TcpClient, TcpServer, Udp };

    // 初始化
    void init();
    void updateProtocolUi(Protocol protocol);
    void updateConnectionStatus(bool connected);

    // 发送数据
    void sendData();

    // 辅助函数
    void appendReceive(const QString& text);
    void updateClientList();
    void handleProtocolData(const QByteArray &rawData, const QString &source);

    // UI 组件
    Ui::MainWindow *ui;

    // 核心模块
    TcpConnection *m_tcpClient = nullptr;
    TcpServerHost *m_tcpServer = nullptr;
    UdpSocket *m_udp = nullptr;
    SendModel *m_sendModel = nullptr;
    ProtocalParser *m_protocolParser = nullptr;

    // 状态
    Protocol m_currentProtocol = Protocol::TcpClient;
    QTimer *m_autoSendTimer = nullptr;
    QString m_saveFilePath;
    //是否自动发送
    bool isAutoSend=false;
    //是否启用协议解析
    bool m_protocolEnabled = false;
};

#endif // MAINWINDOW_H
