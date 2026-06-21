#ifndef SENDMODEL_H
#define SENDMODEL_H

#include <QObject>
#include <QByteArray>
#include <QString>

/**
 * @brief 发送数据模型 - 统一管理发送模式与数据
 *
 * 支持三种发送模式：
 *   - Text: ASCII文本发送
 *   - Hex: 十六进制发送
 *   - File: 文件发送（支持大文件分块传输）
 */
class SendModel : public QObject
{
    Q_OBJECT

public:
    enum class Mode { Text, Hex, File };
    Q_ENUM(Mode)

    explicit SendModel(QObject *parent = nullptr);

    // 当前发送模式
    Mode mode() const { return m_mode; }
    void setMode(Mode mode);

    // 文本/Hex 模式的数据获取
    QByteArray data() const;
    void setText(const QString &text);
    QString text() const { return m_text; }

    // 文件模式
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path);
    qint64 fileSize() const { return m_fileSize; }
    bool hasFile() const { return !m_filePath.isEmpty(); }

    // 读取文件全部内容（用于发送）
    QByteArray readFile() const;

signals:
    void modeChanged(Mode mode);
    void fileLoaded(qint64 size);
    void textChanged();

private:
    Mode m_mode = Mode::Text;
    QString m_text;
    QString m_filePath;
    qint64 m_fileSize = 0;
};

#endif // SENDMODEL_H
