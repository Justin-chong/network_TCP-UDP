#include "protocal.h"
#include <QDebug>

ProtocalParser::ProtocalParser()
{
    m_buffer.clear();
}

// === 接收方向 ===

// 将收到的数据存入缓冲区
void ProtocalParser::appendData(const QByteArray &data)
{
    m_buffer.append(data);
}

// 清空缓冲区
void ProtocalParser::clear()
{
    m_buffer.clear();
}

// 从缓冲区提取一个完整的数据包
bool ProtocalParser::parsePacket(QByteArray &data, quint8 &type)
{
    // 1. 缓冲区必须 >= 最小帧长度
    if (m_buffer.size() < FRAME_MIN_LEN) {
        return false;
    }

    // 2. 找帧头 0xAA 0x55
    int headerIndex = -1;
    int maxIndex = m_buffer.size() - 1;  // 防止 i+1 越界
    for (int i = 0; i < maxIndex; ++i) {
        if ((quint8)m_buffer[i] == FRAME_HEADER_1 &&
            (quint8)m_buffer[i + 1] == FRAME_HEADER_2) {
            headerIndex = i;
            break;
        }
    }

    // 3. 没找到帧头
    if (headerIndex == -1) {
        return false;
    }

    // 4. 帧头不在首位, 切掉前面的垃圾数据
    if (headerIndex > 0) {
        m_buffer = m_buffer.mid(headerIndex);
    }

    // 5. 再次检查最小长度(因为帧头可能刚刚被切到首位, 之前长度不够)
    if (m_buffer.size() < FRAME_MIN_LEN) {
        return false;
    }

    // 6. 读长度字段
    quint8 dataLen = (quint8)m_buffer[FRAME_HEADER_LEN];

    // 7. 计算完整包长度 = 帧头 + 长度字段 + (类型+数据) + 校验
    // 注意: 长度字段表示的是 类型(1) + 数据(N) 的总字节数
    int totalPacketLen = FRAME_HEADER_LEN + FRAME_LEN_LEN + dataLen + FRAME_CHECKSUM_LEN;

    // 8. 数据不完整, 等待下次
    if (m_buffer.size() < totalPacketLen) {
        return false;
    }

    // 9. 取出完整包
    QByteArray packet = m_buffer.left(totalPacketLen);

    // 10. 验证校验和
    quint8 receiveChecksum = (quint8)packet[totalPacketLen - 1];
    quint8 calculatedChecksum = calculateChecksum(packet.left(totalPacketLen - 1));
    if (receiveChecksum != calculatedChecksum) {
        qDebug() << "[Protocol] 校验和错误, 丢弃包";
        m_buffer = m_buffer.mid(totalPacketLen);  // 跳过错误包
        return false;
    }

    // 11. 提取类型和数据
    int dataStart = FRAME_HEADER_LEN + FRAME_LEN_LEN;
    type = (quint8)packet[dataStart];
    data = packet.mid(dataStart + FRAME_TYPE_LEN, dataLen - FRAME_TYPE_LEN);

    // 12. 从缓冲区移除已处理的数据
    m_buffer = m_buffer.mid(totalPacketLen);

    qDebug() << "[Protocol] 解析成功: type=" << QString::number(type, 16)
             << " data=" << data.toHex(' ');
    return true;
}

// === 发送方向 ===

// 打包数据为协议帧
QByteArray ProtocalParser::pack(quint8 type, const QByteArray &data)
{
    QByteArray frame;

    // 1. 帧头
    frame.append((char)FRAME_HEADER_1);
    frame.append((char)FRAME_HEADER_2);

    // 2. 长度字段 = 类型(1) + 数据(N)
    quint8 len = FRAME_TYPE_LEN + data.size();
    frame.append((char)len);

    // 3. 类型
    frame.append((char)type);

    // 4. 数据
    frame.append(data);

    // 5. 校验和(对帧头+长度+类型+数据计算)
    quint8 checksum = calculateChecksum(frame);
    frame.append((char)checksum);

    return frame;
}

// === 私有方法 ===

// 计算校验和: 累加和
quint8 ProtocalParser::calculateChecksum(const QByteArray &data)
{
    quint32 sum = 0;
    for (int i = 0; i < data.size(); ++i) {
        sum += (quint8)data[i];
    }
    return (quint8)(sum & 0xFF);  // 只取低8位
}
