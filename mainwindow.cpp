#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "TcpClient.h"
#include "TcpServer.h"
#include "UDP.h"
#include "SendModel.h"
#include "protocal.h"

#include <QApplication>
#include <QFileDialog>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QTextCursor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("TCP/UDP Debug Assistant v1.0"));

    // 创建核心模块
    m_tcpClient = new TcpConnection(this);
    m_tcpServer = new TcpServerHost(this);
    m_udp = new UdpSocket(this);
    m_sendModel = new SendModel(this);
    m_autoSendTimer = new QTimer(this);
    m_protocolParser = new ProtocalParser();  // 协议解析器

    // 协议解析开关
    if (ui->checkBox_PlainTextMode) {
        connect(ui->checkBox_PlainTextMode, &QCheckBox::toggled,
                this, &MainWindow::onEnableProtocolToggled);
    }

    // 协议切换
    connect(ui->comboBox_protocalType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProtocolTypeChanged);

    // 连接按钮
    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

    // 发送按钮
    connect(ui->btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    // 自动发送定时器
    connect(m_autoSendTimer, &QTimer::timeout, this, &MainWindow::onAutoSendTimeout);

    // 发送模式切换
    connect(ui->radBtn_SendASCII, &QRadioButton::toggled, this, &MainWindow::onSendModeChanged);
    connect(ui->radBtn_SendHex, &QRadioButton::toggled, this, &MainWindow::onSendModeChanged);
    connect(ui->radBtn_SendFile, &QRadioButton::toggled, this, &MainWindow::onSendModeChanged);

    // 文件浏览
    connect(ui->btnSelectFile, &QPushButton::clicked, this, &MainWindow::onBrowseFileClicked);

    // 清除按钮
    connect(ui->btnClearReceive, &QPushButton::clicked, this, &MainWindow::onClearReceiveClicked);
    connect(ui->btnClearSend, &QPushButton::clicked, this, &MainWindow::onClearSendClicked);

    // 保存文件
    connect(ui->checkBox_saveToFile, &QCheckBox::toggled, this, &MainWindow::onSaveToFileToggled);

    // TCP Client 信号
    connect(m_tcpClient, &TcpConnection::connected, this, &MainWindow::onTcpConnected);
    connect(m_tcpClient, &TcpConnection::disconnected, this, &MainWindow::onTcpDisconnected);
    connect(m_tcpClient, &TcpConnection::dataReceived, this, &MainWindow::onTcpDataReceived);
    connect(m_tcpClient, &TcpConnection::errorOccurred, this, &MainWindow::onTcpError);

    // TCP Server 信号
    connect(m_tcpServer, &TcpServerHost::clientConnected, this, &MainWindow::onServerClientConnected);
    connect(m_tcpServer, &TcpServerHost::clientDisconnected, this, &MainWindow::onServerClientDisconnected);
    connect(m_tcpServer, &TcpServerHost::dataReceived, this, &MainWindow::onServerDataReceived);
    connect(m_tcpServer, &TcpServerHost::listeningStateChanged, this, &MainWindow::onServerListeningStateChanged);
    connect(m_tcpServer, &TcpServerHost::errorOccurred, this, &MainWindow::onServerError);

    // UDP 信号
    connect(m_udp, &UdpSocket::dataReceived, this, &MainWindow::onUdpDataReceived);
    connect(m_udp, &UdpSocket::bindingStateChanged, this, &MainWindow::onUdpBindingStateChanged);
    connect(m_udp, &UdpSocket::errorOccurred, this, &MainWindow::onUdpError);

    // 初始化
    init();
    updateProtocolUi(Protocol::TcpClient);
}

MainWindow::~MainWindow()
{
    // 清理资源
    if (m_tcpClient->isConnected()) {
        m_tcpClient->disconnectFromHost();
    }
    if (m_tcpServer->isListening()) {
        m_tcpServer->stop();
    }
    if (m_udp->isBound()) {
        m_udp->close();
    }
    delete ui;
}

void MainWindow::onProtocolTypeChanged(int index)
{
    switch (index) {
    case 0: updateProtocolUi(Protocol::TcpClient); break;
    case 1: updateProtocolUi(Protocol::TcpServer); break;
    case 2: updateProtocolUi(Protocol::Udp); break;
    }
}

void MainWindow::onConnectClicked()
{
    QString host = ui->comboBox_hostAddress->currentText();
    bool portOk;
    quint16 port = ui->lineEdit_port->text().toUShort(&portOk);
    if (!portOk) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a valid port number."));
        return;
    }

    switch (m_currentProtocol) {
    case Protocol::TcpClient: {
        QString remoteHost = ui->lineEdit_remote->text().trimmed();
        if (remoteHost.isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please enter a remote host address."));
            return;
        }
        if (!m_tcpClient->isConnected()) {
            m_tcpClient->connectToHost(remoteHost, port);
        } else {
            m_tcpClient->disconnectFromHost();
        }
        break;
    }
    case Protocol::TcpServer: {
        if (!m_tcpServer->isListening()) {
            if (!m_tcpServer->listen(host, port)) {
                QMessageBox::critical(this, tr("Error"), tr("Failed to start server."));
            }
        } else {
            m_tcpServer->stop();
        }
        break;
    }
    case Protocol::Udp: {
        if (!m_udp->isBound()) {
            if (!m_udp->bind(port)) {
                QMessageBox::critical(this, tr("Error"), tr("Failed to bind UDP port."));
            }
        } else {
            m_udp->close();
        }
        break;
    }
    }
}

void MainWindow::onSendClicked()
{
    QString text = ui->textEdit_Send->toPlainText();
    QString filePath = ui->lineEdit_SendFilePath->text();

    // 启动/停止自动发送
    if (ui->btnSend->text() == tr("发送")) {
        // 启动模式
        bool autoSend = ui->checkBox_AutoSend->isChecked();

        if (autoSend) {
            bool ok;
            int interval = ui->lineEdit_AutoSend->text().toInt(&ok);
            if (!ok || interval <= 0) {
                // 错误提示
                return;
            }
            m_autoSendTimer->start(interval);
            ui->btnSend->setText(tr("停止"));
        }

        sendData();  // 立即发送一次

    } else {
        // 停止模式
        if (m_autoSendTimer->isActive()) {
            m_autoSendTimer->stop();
        }
        ui->btnSend->setText(tr("发送"));
    }
}

void MainWindow::sendData()
{
    // 发送前同步文本内容到 SendModel
    if (m_sendModel->mode() != SendModel::Mode::File) {
        m_sendModel->setText(ui->textEdit_Send->toPlainText());
    }

    switch (m_currentProtocol) {
    case Protocol::TcpClient: {
        if (!m_tcpClient->isConnected()) return;
        QByteArray data = m_sendModel->data();
        if (m_sendModel->mode() == SendModel::Mode::File && m_sendModel->hasFile()) {
            data = m_sendModel->readFile();
        }
        if (data.isEmpty()) return;
        // 启用协议解析时: 输入必须是完整协议帧, 解析后只发送数据部分
        if (m_protocolEnabled && m_protocolParser) {
            qDebug() << "[Protocol Send] 原始数据:" << data.toHex(' ').toUpper()
                     << "size=" << data.size();
            m_protocolParser->clear();
            m_protocolParser->appendData(data);
            QByteArray payload;
            quint8 type = 0;
            if (m_protocolParser->parsePacket(payload, type)) {
                // 解析成功, 发送数据部分
                m_tcpClient->sendData(payload);
                QString display = ui->radBtn_ReceiveASCII->isChecked()
                                  ? QString::fromUtf8(payload)
                                  : payload.toHex(' ').toUpper();
                appendReceive(tr("[Protocol Send] type=0x%1, %2 bytes\n内容: %3")
                                  .arg(QString::number(type, 16).toUpper().rightJustified(2, '0'))
                                  .arg(payload.size())
                                  .arg(display));
            } else {
                qDebug() << "[Protocol Send] 解析失败, 缓冲区内容:" << m_protocolParser->bufferSize();
                appendReceive(tr("[Protocol Send] 协议帧格式错误, 请检查校验和"));
            }
            return;
        }
        m_tcpClient->sendData(data);
        break;
    }
    case Protocol::TcpServer: {
        if (!m_tcpServer->isListening()) return;
        QByteArray data = m_sendModel->data();
        if (m_sendModel->mode() == SendModel::Mode::File && m_sendModel->hasFile()) {
            data = m_sendModel->readFile();
        }
        if (data.isEmpty()) return;
        // 启用协议解析时: 输入必须是完整协议帧, 解析后只发送数据部分
        if (m_protocolEnabled && m_protocolParser) {
            m_protocolParser->clear();
            m_protocolParser->appendData(data);
            QByteArray payload;
            quint8 type = 0;
            if (m_protocolParser->parsePacket(payload, type)) {
                m_tcpServer->broadcast(payload);
                QString display = ui->radBtn_ReceiveASCII->isChecked()
                                  ? QString::fromUtf8(payload)
                                  : payload.toHex(' ').toUpper();
                appendReceive(tr("[Protocol Send] type=0x%1, %2 bytes\n内容: %3")
                                  .arg(QString::number(type, 16).toUpper().rightJustified(2, '0'))
                                  .arg(payload.size())
                                  .arg(display));
            } else {
                appendReceive(tr("[Protocol Send] 协议帧格式错误, 请检查校验和"));
            }
            return;
        }
        m_tcpServer->broadcast(data);
        break;
    }
    case Protocol::Udp: {
        if (!m_udp->isBound()) return;
        QByteArray data = m_sendModel->data();
        if (m_sendModel->mode() == SendModel::Mode::File && m_sendModel->hasFile()) {
            data = m_sendModel->readFile();
        }
        if (data.isEmpty()) return;

        QString remoteHost = ui->lineEdit_remote->text().trimmed();
        bool portOk;
        quint16 remotePort = ui->lineEdit_remotePort->text().toUShort(&portOk);
        if (remoteHost.isEmpty() || !portOk) {
            QMessageBox::warning(this, tr("Warning"), tr("Please enter remote host and port."));
            return;
        }
        // 启用协议解析时: 输入必须是完整协议帧, 解析后只发送数据部分
        if (m_protocolEnabled && m_protocolParser) {
            m_protocolParser->clear();
            m_protocolParser->appendData(data);
            QByteArray payload;
            quint8 type = 0;
            if (m_protocolParser->parsePacket(payload, type)) {
                m_udp->sendTo(remoteHost, remotePort, payload);
                QString display = ui->radBtn_ReceiveASCII->isChecked()
                                  ? QString::fromUtf8(payload)
                                  : payload.toHex(' ').toUpper();
                appendReceive(tr("[Protocol Send] type=0x%1, %2 bytes\n内容: %3")
                                  .arg(QString::number(type, 16).toUpper().rightJustified(2, '0'))
                                  .arg(payload.size())
                                  .arg(display));
            } else {
                appendReceive(tr("[Protocol Send] 协议帧格式错误, 请检查校验和"));
            }
            return;
        }
        m_udp->sendTo(remoteHost, remotePort, data);
        break;
    }
    }
}

void MainWindow::onAutoSendTimeout()
{
    bool isActive = false;
    switch (m_currentProtocol) {
    case Protocol::TcpClient: isActive = m_tcpClient->isConnected(); break;
    case Protocol::TcpServer: isActive = m_tcpServer->isListening(); break;
    case Protocol::Udp: isActive = m_udp->isBound(); break;
    }

    if (!isActive) {
        m_autoSendTimer->stop();
        ui->btnSend->setText(tr("Send"));
        appendReceive(tr("[Notice] Connection lost, auto-send stopped."));
        return;
    }
    sendData();
}

void MainWindow::onTcpConnected()
{
    updateConnectionStatus(true);
    appendReceive(tr("[Connected] TCP client connected successfully."));
}

void MainWindow::onTcpDisconnected()
{
    updateConnectionStatus(false);
    appendReceive(tr("[Disconnected] TCP client disconnected."));
}

void MainWindow::onTcpDataReceived(const QByteArray &data)
{
    // 启用协议解析时, 走协议解析流程
    if (m_protocolEnabled) {
        handleProtocolData(data, "TCP");
        return;
    }

    QString display;
    if (ui->radBtn_ReceiveASCII->isChecked()) {
        display = QString::fromUtf8(data);
    } else {
        display = data.toHex(' ').toUpper();
    }

    QString info = tr("[Received from TCP] %1 bytes\nContent: %2")
                       .arg(data.size())
                       .arg(display);
    appendReceive(info);

    // 保存到文件
    if (ui->checkBox_saveToFile->isChecked() && !m_saveFilePath.isEmpty()) {
        QFile file(m_saveFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream out(&file);
            out << info << Qt::endl;
        }
    }
}

void MainWindow::onTcpError(const QString &error)
{
    appendReceive(tr("[Error] TCP: %1").arg(error));
}

void MainWindow::onServerClientConnected(int clientId, const QString &address)
{
    //连接客户端成功
    ui->btnConnect->setText("停止");
    appendReceive(tr("[Server] Client %1 connected from %2").arg(clientId).arg(address));
    updateClientList();
}

void MainWindow::onServerClientDisconnected(int clientId)
{
    //有客户端断开连接
    ui->btnConnect->setText("开始");
    appendReceive(tr("[Server] Client %1 disconnected").arg(clientId));
    updateClientList();
}

void MainWindow::onServerDataReceived(int clientId, const QByteArray &data)
{
    // 启用协议解析时, 走协议解析流程
    if (m_protocolEnabled) {
        handleProtocolData(data, QString("Client %1").arg(clientId));
        return;
    }

    QString display;
    if (ui->radBtn_ReceiveASCII->isChecked()) {
        display = QString::fromUtf8(data);
    } else {
        display = data.toHex(' ').toUpper();
    }

    QString info = tr("[Received from Client %1] %2 bytes\nContent: %3")
                       .arg(clientId)
                       .arg(data.size())
                       .arg(display);
    appendReceive(info);

    if (ui->checkBox_saveToFile->isChecked() && !m_saveFilePath.isEmpty()) {
        QFile file(m_saveFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream out(&file);
            out << info << Qt::endl;
        }
    }
}

void MainWindow::onServerListeningStateChanged(bool listening)
{
    if (listening) {
        appendReceive(tr("[Server] Listening started."));
    } else {
        appendReceive(tr("[Server] Listening stopped."));
    }
}

void MainWindow::onServerError(const QString &error)
{
    appendReceive(tr("[Error] Server: %1").arg(error));
}

void MainWindow::onUdpDataReceived(const QByteArray &data, const QString &senderAddress, quint16 senderPort)
{
    // 启用协议解析时, 走协议解析流程
    if (m_protocolEnabled) {
        handleProtocolData(data, QString("UDP %1:%2").arg(senderAddress).arg(senderPort));
        return;
    }

    QString display;
    if (ui->radBtn_ReceiveASCII->isChecked()) {
        display = QString::fromUtf8(data);
    } else {
        display = data.toHex(' ').toUpper();
    }

    QString info = tr("[Received from UDP %1:%2] %3 bytes\nContent: %4")
                       .arg(senderAddress)
                       .arg(senderPort)
                       .arg(data.size())
                       .arg(display);
    appendReceive(info);

    if (ui->checkBox_saveToFile->isChecked() && !m_saveFilePath.isEmpty()) {
        QFile file(m_saveFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream out(&file);
            out << info << Qt::endl;
        }
    }
}

void MainWindow::onUdpBindingStateChanged(bool bound)
{
    if (bound) {
        appendReceive(tr("[UDP] Bound to port %1").arg(m_udp->boundPort()));
    } else {
        appendReceive(tr("[UDP] Unbound."));
    }
}

void MainWindow::onUdpError(const QString &error)
{
    appendReceive(tr("[Error] UDP: %1").arg(error));
}

void MainWindow::onSendModeChanged(bool checked)
{
    if (!checked) return;

    if (ui->radBtn_SendASCII->isChecked()) {
        m_sendModel->setMode(SendModel::Mode::Text);
        ui->stackedWidget->setCurrentIndex(1); // 文本页面
    } else if (ui->radBtn_SendHex->isChecked()) {
        m_sendModel->setMode(SendModel::Mode::Hex);
        ui->stackedWidget->setCurrentIndex(1); // 文本页面
    } else if (ui->radBtn_SendFile->isChecked()) {
        m_sendModel->setMode(SendModel::Mode::File);
        ui->stackedWidget->setCurrentIndex(0); // 文件页面
    }
}

void MainWindow::onBrowseFileClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Select File to Send"),
        QDir::homePath(),
        tr("All Files (*)")
    );

    if (filePath.isEmpty()) return;

    ui->lineEdit_SendFilePath->setText(filePath);
    m_sendModel->setFilePath(filePath);
    ui->label_SendByteSize->setText(tr("File size: %1 bytes").arg(m_sendModel->fileSize()));
}

void MainWindow::onClearReceiveClicked()
{
    ui->textEdit_Receive->clear();
}

void MainWindow::onClearSendClicked()
{
    ui->textEdit_Send->clear();
    m_sendModel->setText(ui->textEdit_Send->toPlainText());
}

void MainWindow::onSaveToFileToggled(bool checked)
{
    if (checked) {
        QString filePath = QFileDialog::getSaveFileName(
            this, tr("Select Save Path"), QDir::homePath(), tr("Text Files (*.txt)")
        );
        if (filePath.isEmpty()) {
            ui->checkBox_saveToFile->setChecked(false);
            return;
        }
        m_saveFilePath = filePath;
        appendReceive(tr("[Info] Save path set: %1").arg(filePath));
    } else {
        m_saveFilePath.clear();
    }
}

void MainWindow::init()
{
    // 填充协议下拉框
    ui->comboBox_protocalType->addItem(tr("TCP Client"));
    ui->comboBox_protocalType->addItem(tr("TCP Server"));
    ui->comboBox_protocalType->addItem(tr("UDP"));

    // 填充主机地址
    ui->comboBox_hostAddress->addItem("127.0.0.1");
    ui->comboBox_hostAddress->addItem("0.0.0.0");

    // 获取本机所有 IPv4 地址
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : addresses) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost) {
            QString addrStr = addr.toString();
            if (ui->comboBox_hostAddress->findText(addrStr) == -1) {
                ui->comboBox_hostAddress->addItem(addrStr);
            }
        }
    }

    // 默认端口
    ui->lineEdit_port->setText("8888");

    // 设置初始协议 UI
    updateProtocolUi(Protocol::TcpClient);

    // 设置默认发送/接收模式（确保至少有一个被选中）
    if (!ui->radBtn_SendASCII->isChecked() &&
        !ui->radBtn_SendHex->isChecked() &&
        !ui->radBtn_SendFile->isChecked()) {
        ui->radBtn_SendASCII->setChecked(true);
    }

    if (!ui->radBtn_ReceiveASCII->isChecked() &&
        !ui->radBtn_ReceiveHex->isChecked()) {
        ui->radBtn_ReceiveASCII->setChecked(true);
    }
}

void MainWindow::updateProtocolUi(Protocol protocol)
{
    m_currentProtocol = protocol;

    switch (protocol) {
    case Protocol::TcpClient:
        ui->btnConnect->setText(tr("连接"));
        ui->label_remote->setText(tr("远程主机地址"));
        ui->lineEdit_remote->setEnabled(true);
        ui->lineEdit_remotePort->setEnabled(true);
        ui->comboBox_hostAddress->setEnabled(false);
        ui->comboBox_clients->setEnabled(false);
        break;
    case Protocol::TcpServer:
        ui->btnConnect->setText(tr("开始"));
        ui->label_remote->setText(tr("远程主机地址"));
        ui->lineEdit_remote->setEnabled(false);
        ui->lineEdit_remotePort->setEnabled(false);
        ui->lineEdit_remote->clear();
        ui->lineEdit_remotePort->clear();
        ui->comboBox_hostAddress->setEnabled(true);
        ui->comboBox_clients->setEnabled(true);
        break;
    case Protocol::Udp:
        ui->btnConnect->setText(tr("绑定"));
        ui->label_remote->setText(tr("远程主机地址"));
        ui->lineEdit_remote->setEnabled(false);
        ui->lineEdit_remotePort->setEnabled(false);
        ui->lineEdit_remote->clear();
        ui->lineEdit_remotePort->clear();
        ui->comboBox_hostAddress->setEnabled(false);
        ui->comboBox_clients->setEnabled(false);
        break;
    }
}

void MainWindow::updateConnectionStatus(bool connected)
{
    if (m_currentProtocol == Protocol::TcpClient) {
        ui->btnConnect->setText(connected ? tr("断开") : tr("连接"));
    } else if (m_currentProtocol == Protocol::TcpServer) {
        ui->btnConnect->setText(connected ? tr("停止") : tr("开始"));
    } else if (m_currentProtocol == Protocol::Udp) {
        ui->btnConnect->setText(connected ? tr("非绑定") : tr("绑定"));
    }
    ui->btnConnect->setEnabled(true);
}

void MainWindow::updateClientList()
{
    auto clients = m_tcpServer->connectedClients();
    ui->comboBox_clients->clear();
    for (const auto &c : clients) {
        ui->comboBox_clients->addItem(QString(tr("ID:%1 - %2")).arg(c.first).arg(c.second));
    }
}

void MainWindow::appendReceive(const QString &text)
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyy-MM-dd hh:mm:ss.zzz");
    ui->textEdit_Receive->append(QString("[%1] %2").arg(timestamp).arg(text));
    // 自动滚动到底部, 显示最新内容
    QTextCursor cursor = ui->textEdit_Receive->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEdit_Receive->setTextCursor(cursor);
}

// ===========================================
// 协议解析相关函数
// ===========================================

// 启用/禁用协议解析
void MainWindow::onEnableProtocolToggled(bool checked)
{
    m_protocolEnabled = checked;
    if (m_protocolParser) {
        m_protocolParser->clear();  // 清空缓冲区
    }
    // 启用协议解析时, 自动切换到 HEX 发送模式
    if (checked && ui->radBtn_SendHex) {
        ui->radBtn_SendHex->setChecked(true);
    }
    appendReceive(tr("[Protocol] %1").arg(checked ? tr("Enabled") : tr("Disabled")));
}

// 处理原始数据（喂给协议解析器，返回解析后的数据部分）
//   rawData: 原始接收到的字节流
//   source:  数据来源描述(如 "TCP", "Client 1", "UDP 192.168.1.100:8888")
void MainWindow::handleProtocolData(const QByteArray &rawData, const QString &source)
{
    if (!m_protocolParser) return;

    // 1. 喂数据给协议解析器
    m_protocolParser->appendData(rawData);

    // 2. 循环解析（可能一次解析出多帧）
    QByteArray data;
    quint8 type = 0;
    int frameCount = 0;

    while (m_protocolParser->parsePacket(data, type)) {
        frameCount++;
        // 3. 解析成功, 解析数据(根据类型显示)
        QString typeName;
        switch (type) {
            case ProtocolCmd::Heartbeat:  typeName = "心跳"; break;
            case ProtocolCmd::DataFrame:  typeName = "数据帧"; break;
            case ProtocolCmd::AckFrame:   typeName = "应答帧"; break;
            case ProtocolCmd::ErrorFrame: typeName = "错误帧"; break;
            default: typeName = QString("自定义(0x%1)").arg(QString::number(type, 16).toUpper());
        }

        // 显示内容(根据接收设置选择 ASCII/HEX)
        QString display;
        if (ui->radBtn_ReceiveASCII->isChecked()) {
            display = QString::fromUtf8(data);
        } else {
            display = data.toHex(' ').toUpper();
        }

        QString info = tr("[Protocol from %1] 第%2帧  类型:0x%3(%4)  数据:%5字节\n内容: %6")
                           .arg(source)
                           .arg(frameCount)
                           .arg(QString::number(type, 16).toUpper().rightJustified(2, '0'))
                           .arg(typeName)
                           .arg(data.size())
                           .arg(display);
        appendReceive(info);

        // 保存到文件
        if (ui->checkBox_saveToFile->isChecked() && !m_saveFilePath.isEmpty()) {
            QFile file(m_saveFilePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                QTextStream out(&file);
                out << info << Qt::endl;
            }
        }
    }
}
