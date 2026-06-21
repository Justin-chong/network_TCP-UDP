#include "SendModel.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>

SendModel::SendModel(QObject *parent)
    : QObject(parent)
{}

void SendModel::setMode(Mode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        emit modeChanged(mode);
    }
}

QByteArray SendModel::data() const
{
    if (m_mode == Mode::Hex) {
        // 解析十六进制字符串为字节数组
        QString trimmed = m_text.trimmed();
        QByteArray result;
        // 去除空格和分隔符
        QString clean;
        for (const QChar &ch : trimmed) {
            if (!ch.isSpace() && ch != QLatin1Char('-'))
                clean += ch;
        }
        // 确保偶数个字符
        if (clean.length() % 2 != 0)
            clean.prepend('0');
        // 每两个字符转为一个字节
        for (int i = 0; i + 1 < clean.length(); i += 2) {
            bool ok;
            uchar val = clean.mid(i, 2).toUInt(&ok, 16);
            if (ok)
                result.append(static_cast<char>(val));
        }
        return result;
    }
    // Text 模式
    return m_text.toUtf8();
}

void SendModel::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        emit textChanged();
    }
}

void SendModel::setFilePath(const QString &path)
{
    m_filePath = path;
    if (!path.isEmpty()) {
        QFileInfo fi(path);
        m_fileSize = fi.size();
        emit fileLoaded(m_fileSize);
    } else {
        m_fileSize = 0;
    }
}

QByteArray SendModel::readFile() const
{
    if (m_filePath.isEmpty())
        return {};

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << m_filePath;
        return {};
    }
    return file.readAll();
}
