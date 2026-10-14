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

    auto msToTimeStr = [](qint64 ms) {
        QTime t(0, 0, 0);
        t = t.addMSecs(static_cast<int>(ms));
        return QString("%1.%2").arg(t.toString("hh:mm:ss")).arg(ms % 1000, 3, 10, QChar('0'));
    };

    QString startStr = msToTimeStr(startMs);
    QString durationStr = msToTimeStr(m_targetDurationMs);

    QStringList args;
    args << "-y";

    if (mode == TrimMode::LosslessStreamCopy) {
        // Fast seek before -i for rapid stream copying
        args << "-ss" << startStr
             << "-i" << inputFile
             << "-t" << durationStr
             << "-c" << "copy"
             << "-avoid_negative_ts" << "make_zero"
             << "-movflags" << "+faststart"
             << outputFile;
        emit statusUpdated("Performing instant lossless cut...");
    } else {
        // Accurate seek with re-encoding
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
            emit trimFinished(false, QString("Trim failed with exit code %1.").arg(exitCode));
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