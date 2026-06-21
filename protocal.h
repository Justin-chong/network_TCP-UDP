#ifndef PROTOCAL_H
#define PROTOCAL_H

#include <QByteArray>
#include <QObject>

// 协议帧格式定义
// 帧结构: [AA 55] [LEN] [TYPE] [DATA...] [CHECKSUM]
//   帧头:    2字节, 固定 0xAA 0x55
//   长度:    1字节, 表示 (类型 + 数据) 的字节数
//   类型:    1字节, 命令类型
//   数据:    N字节, 实际数据
//   校验和:  1字节, 累加和校验(帧头+长度+类型+数据)

#define FRAME_HEADER_1 0xAA
#define FRAME_HEADER_2 0x55
#define FRAME_HEADER_LEN 2
#define FRAME_LEN_LEN 1
#define FRAME_TYPE_LEN 1
#define FRAME_CHECKSUM_LEN 1
#define FRAME_MIN_LEN (FRAME_HEADER_LEN + FRAME_LEN_LEN + FRAME_TYPE_LEN + FRAME_CHECKSUM_LEN)
#define FRAME_MAX_DATA_LEN 255

// 常用命令类型
namespace ProtocolCmd {
    const quint8 Heartbeat    = 0x00;  // 心跳
    const quint8 DataFrame    = 0x01;  // 数据帧
    const quint8 AckFrame     = 0x02;  // 应答帧
    const quint8 ErrorFrame   = 0xFF;  // 错误帧
}

// 协议解析类 - 用于处理 TCP 黏包/半包, 自动校验
class ProtocalParser
{
public:
    ProtocalParser();

    // === 接收方向 ===
    // 喂入新收到的数据(支持多次喂入, 自动处理黏包/半包)
    void appendData(const QByteArray &data);

    // 尝试从缓冲区解析一个完整的数据包
    //   data: 输出参数, 成功时填入数据部分(不含帧头/长度/校验)
    //   type: 输出参数, 命令类型
    //   返回 true 表示解析成功
    bool parsePacket(QByteArray &data, quint8 &type);

    // 清空缓冲区
    void clear();

    // === 发送方向 ===
    // 打包数据为协议帧
    //   type: 命令类型
    //   data: 数据内容
    //   返回完整的协议帧字节流
    QByteArray pack(quint8 type, const QByteArray &data);

    // === 状态查询 ===
    int bufferSize() const { return m_buffer.size(); }

private:
    // 计算校验和: 累加和
    quint8 calculateChecksum(const QByteArray &data);

    // 缓冲区
    QByteArray m_buffer;
};

#endif // PROTOCAL_H
