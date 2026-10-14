#pragma once

#include <QObject>
#include <QProcess>

class TrimEngine : public QObject {
    Q_OBJECT
public:
    enum class TrimMode {
        LosslessStreamCopy,  // Fast, instant, snaps to keyframe
        AccurateTranscode    // Millisecond exact, re-encodes
    };

    explicit TrimEngine(QString ffmpegPath, QObject *parent = nullptr);
    ~TrimEngine() override;

    void startTrim(const QString &inputFile, const QString &outputFile,
                   qint64 startMs, qint64 endMs, TrimMode mode);
    void cancel();

signals:
    void statusUpdated(const QString &status);
    void progressUpdated(int percent);
    void trimFinished(bool success, const QString &errorMessage);

private:
    void parseProgress(const QString &line);

    QString m_ffmpegPath;
    QProcess *m_process{nullptr};
    qint64 m_targetDurationMs{0};
    bool m_cancelled{false};
};