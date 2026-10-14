#include "TrimEngine.h"
#include <QRegularExpression>
#include <QTime>

TrimEngine::TrimEngine(QString ffmpegPath, QObject *parent)
    : QObject(parent)
    , m_ffmpegPath(std::move(ffmpegPath)) {}

TrimEngine::~TrimEngine() {
    cancel();
}

void TrimEngine::cancel() {
    m_cancelled = true;
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void TrimEngine::startTrim(const QString &inputFile, const QString &outputFile,
                           qint64 startMs, qint64 endMs, TrimMode mode) {
    m_cancelled = false;
    m_targetDurationMs = std::max<qint64>(1, endMs - startMs);

    if (m_process) m_process->deleteLater();
    m_process = new QProcess(this);

    // Format timestamps as standard seconds.milliseconds (e.g., "12.350")
    double startSec = static_cast<double>(startMs) / 1000.0;
    double durationSec = static_cast<double>(m_targetDurationMs) / 1000.0;

    QString startStr = QString::number(startSec, 'f', 3);
    QString durationStr = QString::number(durationSec, 'f', 3);

    QStringList args;
    args << "-y";

    if (mode == TrimMode::LosslessStreamCopy) {
        // Fast seek before input + stream copy with zero timestamp normalization
        args << "-ss" << startStr
             << "-i" << inputFile
             << "-t" << durationStr
             << "-c" << "copy"
             << "-avoid_negative_ts" << "make_zero"
             << "-movflags" << "+faststart"
             << outputFile;
        emit statusUpdated("Performing instant lossless cut...");
    } else {
        // Precise seek + full transcode
        args << "-ss" << startStr
             << "-i" << inputFile
             << "-t" << durationStr
             << "-c:v" << "libx264"
             << "-preset" << "fast"
             << "-crf" << "18"
             << "-c:a" << "aac"
             << "-b:a" << "192k"
             << "-movflags" << "+faststart"
             << outputFile;
        emit statusUpdated("Performing frame-accurate transcode...");
    }

    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        while (m_process && m_process->canReadLine()) {
            parseProgress(QString::fromUtf8(m_process->readLine()));
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (m_cancelled) {
            emit trimFinished(false, "Operation aborted by user.");
            return;
        }

        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            emit progressUpdated(100);
            emit trimFinished(true, QString());
        } else {
            QString errOutput;
            if (m_process) errOutput = QString::fromUtf8(m_process->readAllStandardError());
            emit trimFinished(false, QString("FFmpeg exited with code %1:\n%2").arg(exitCode).arg(errOutput.right(400)));
        }
    });

    m_process->start(m_ffmpegPath, args);
}

void TrimEngine::parseProgress(const QString &line) {
    static QRegularExpression re(R"(time=(\d+):(\d+):(\d+\.\d+))");
    auto match = re.match(line);
    if (match.hasMatch() && m_targetDurationMs > 0) {
        qint64 hours = match.captured(1).toLongLong();
        qint64 mins = match.captured(2).toLongLong();
        double secs = match.captured(3).toDouble();
        qint64 currentMs = (hours * 3600000) + (mins * 60000) + static_cast<qint64>(secs * 1000.0);

        int pct = qBound(0, static_cast<int>((currentMs * 100) / m_targetDurationMs), 99);
        emit progressUpdated(pct);
    }
}