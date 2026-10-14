#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QTime>
#include <QProgressDialog>

static QString formatMs(qint64 ms) {
    QTime t(0, 0, 0);
    t = t.addMSecs(static_cast<int>(ms));
    return QString("%1.%2").arg(t.toString("hh:mm:ss")).arg((ms % 1000) / 100);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_ffmpegManager(new FFmpegManager(this))
    , m_updateChecker(new UpdateChecker("1.0.0", this)) {
    setupUi();
    setupMenuBar();
    applyStyles();
    setAcceptDrops(true);

    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoOutput(m_videoWidget);

    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onMediaPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onMediaDurationChanged);

    connect(m_updateChecker, &UpdateChecker::updateAvailable, this, &MainWindow::onUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::upToDate, this, [this](const QString &ver) {
        QMessageBox::information(this, "Up to Date", QString("You are running the latest version (v%1).").arg(ver));
    });
    connect(m_updateChecker, &UpdateChecker::checkFailed, this, [this](const QString &err) {
        QMessageBox::warning(this, "Update Check", "Unable to check for updates:\n" + err);
    });

    if (m_ffmpegManager->checkFFmpegAvailability()) {
        m_statusLabel->setText("FFmpeg detected.");
    }

    m_updateChecker->checkForUpdates(false);
}

MainWindow::~MainWindow() {
    if (m_player) m_player->stop();
}

void MainWindow::setupMenuBar() {
    auto *menuHelp = menuBar()->addMenu("&Help");

    auto *actCheckUpdates = menuHelp->addAction("Check for Updates...");
    connect(actCheckUpdates, &QAction::triggered, this, &MainWindow::onCheckForUpdatesTriggered);

    menuHelp->addSeparator();

    auto *actAbout = menuHelp->addAction("About Video Trimmer Pro");
    connect(actAbout, &QAction::triggered, this, &MainWindow::onAboutTriggered);
}

void MainWindow::setupUi() {
    setWindowTitle("Video Trimmer Pro (Qt 6 / C++20 / FFmpeg)");
    setWindowIcon(QIcon("app_icon.ico"));
    resize(960, 680);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // Header Toolbar
    auto *topLayout = new QHBoxLayout();
    m_btnOpenFile = new QPushButton("Open Video File...", this);
    connect(m_btnOpenFile, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    topLayout->addWidget(m_btnOpenFile);
    topLayout->addStretch();
    rootLayout->addLayout(topLayout);

    // Media Preview
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumSize(640, 360);
    m_videoWidget->setStyleSheet("background-color: #000; border-radius: 4px;");
    rootLayout->addWidget(m_videoWidget, 1);

    // Timeline & Scrubbing
    m_rangeSlider = new RangeSlider(this);
    connect(m_rangeSlider, &RangeSlider::currentPosChanged, m_player, &QMediaPlayer::setPosition);
    connect(m_rangeSlider, &RangeSlider::rangeChanged, this, &MainWindow::onRangeChanged);
    rootLayout->addWidget(m_rangeSlider);

    // In / Out Points Controls
    auto *controlsLayout = new QHBoxLayout();
    m_btnPlayPause = new QPushButton("Play", this);
    m_btnPlayPause->setEnabled(false);
    connect(m_btnPlayPause, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);

    m_btnSetStart = new QPushButton("Set In-Point [", this);
    m_btnSetStart->setEnabled(false);
    connect(m_btnSetStart, &QPushButton::clicked, this, &MainWindow::onSetStartToCurrent);

    m_btnSetEnd = new QPushButton("Set Out-Point ]", this);
    m_btnSetEnd->setEnabled(false);
    connect(m_btnSetEnd, &QPushButton::clicked, this, &MainWindow::onSetEndToCurrent);

    m_lblTrimPoints = new QLabel("In: 00:00:00.0 | Out: 00:00:00.0", this);
    m_lblTrimPoints->setStyleSheet("font-weight: bold;");

    m_lblDurationInfo = new QLabel("Selection: 00:00:00.0", this);
    m_lblDurationInfo->setStyleSheet("color: #0D6EFD; font-weight: bold;");

    controlsLayout->addWidget(m_btnPlayPause);
    controlsLayout->addWidget(m_btnSetStart);
    controlsLayout->addWidget(m_btnSetEnd);
    controlsLayout->addWidget(m_lblTrimPoints);
    controlsLayout->addStretch();
    controlsLayout->addWidget(m_lblDurationInfo);
    rootLayout->addLayout(controlsLayout);

    // Output Destination
    auto *destLayout = new QHBoxLayout();
    auto *destLabel = new QLabel("Output File:", this);
    destLabel->setStyleSheet("font-weight: bold;");
    m_editOutputPath = new QLineEdit(this);
    m_editOutputPath->setPlaceholderText("Select output file path (*.mp4)...");
    m_btnBrowseOutput = new QPushButton("Browse...", this);
    connect(m_btnBrowseOutput, &QPushButton::clicked, this, &MainWindow::onBrowseOutputClicked);

    destLayout->addWidget(destLabel);
    destLayout->addWidget(m_editOutputPath);
    destLayout->addWidget(m_btnBrowseOutput);
    rootLayout->addLayout(destLayout);

    // Mode Selector & Execution Bar
    auto *actionLayout = new QHBoxLayout();
    auto *modeLabel = new QLabel("Cut Mode:", this);
    modeLabel->setStyleSheet("font-weight: bold;");

    m_comboMode = new QComboBox(this);
    m_comboMode->addItem("Lossless Copy (Instant, Keyframe Snapped)", static_cast<int>(TrimEngine::TrimMode::LosslessStreamCopy));
    m_comboMode->addItem("Accurate Transcode (Frame-Exact, Re-encode)", static_cast<int>(TrimEngine::TrimMode::AccurateTranscode));

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_btnStartTrim = new QPushButton("Export Trim", this);
    m_btnStartTrim->setStyleSheet("background-color: #0078D4; color: white; font-weight: bold; padding: 8px 18px;");
    m_btnStartTrim->setEnabled(false);
    connect(m_btnStartTrim, &QPushButton::clicked, this, &MainWindow::onStartTrimClicked);

    m_btnCancel = new QPushButton("Cancel", this);
    m_btnCancel->setEnabled(false);
    connect(m_btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelClicked);

    actionLayout->addWidget(modeLabel);
    actionLayout->addWidget(m_comboMode);
    actionLayout->addWidget(m_progressBar, 1);
    actionLayout->addWidget(m_btnStartTrim);
    actionLayout->addWidget(m_btnCancel);
    rootLayout->addLayout(actionLayout);

    m_statusLabel = new QLabel("Drop a video file to begin.", this);
    m_statusLabel->setStyleSheet("color: #666;");
    rootLayout->addWidget(m_statusLabel);
}

void MainWindow::loadVideo(const QString &path) {
    m_loadedFilePath = path;
    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(path));

    QFileInfo fi(path);
    QString suggestedOut = fi.absolutePath() + "/" + fi.completeBaseName() + "_trimmed.mp4";
    m_editOutputPath->setText(suggestedOut);

    m_btnPlayPause->setEnabled(true);
    m_btnPlayPause->setText("Play");
    m_btnSetStart->setEnabled(true);
    m_btnSetEnd->setEnabled(true);
    m_btnStartTrim->setEnabled(true);
    m_statusLabel->setText("Ready: " + fi.fileName());
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    for (const auto &url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            loadVideo(url.toLocalFile());
            break;
        }
    }
}

void MainWindow::onOpenFileClicked() {
    QString file = QFileDialog::getOpenFileName(
        this, "Select Video File", QString(),
        "Video Files (*.mp4 *.mkv *.avi *.mov *.ts *.flv *.webm *.wmv)"
    );
    if (!file.isEmpty()) loadVideo(file);
}

void MainWindow::onBrowseOutputClicked() {
    QString out = QFileDialog::getSaveFileName(this, "Select Output File", m_editOutputPath->text(), "MP4 Video (*.mp4)");
    if (!out.isEmpty()) {
        if (!out.endsWith(".mp4", Qt::CaseInsensitive)) out += ".mp4";
        m_editOutputPath->setText(out);
    }
}

void MainWindow::onPlayPauseClicked() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_btnPlayPause->setText("Play");
    } else {
        m_player->play();
        m_btnPlayPause->setText("Pause");
    }
}

void MainWindow::onMediaPositionChanged(qint64 position) {
    m_rangeSlider->setCurrentPos(position);
}

void MainWindow::onMediaDurationChanged(qint64 duration) {
    m_rangeSlider->setRange(0, duration);
    updateTimestampLabels();
}

void MainWindow::onSetStartToCurrent() {
    m_rangeSlider->setStartPos(m_player->position());
    updateTimestampLabels();
}

void MainWindow::onSetEndToCurrent() {
    m_rangeSlider->setEndPos(m_player->position());
    updateTimestampLabels();
}

void MainWindow::onRangeChanged(qint64, qint64) {
    updateTimestampLabels();
}

void MainWindow::updateTimestampLabels() {
    qint64 s = m_rangeSlider->startPos();
    qint64 e = m_rangeSlider->endPos();
    m_lblTrimPoints->setText(QString("In: %1 | Out: %2").arg(formatMs(s), formatMs(e)));
    m_lblDurationInfo->setText(QString("Selection: %1").arg(formatMs(std::max<qint64>(0, e - s))));
}

void MainWindow::onStartTrimClicked() {
    QString outPath = m_editOutputPath->text().trimmed();
    if (outPath.isEmpty()) {
        QMessageBox::warning(this, "Missing Path", "Please specify an output file destination.");
        return;
    }

    m_player->stop();
    m_btnPlayPause->setText("Play");

    m_btnStartTrim->setEnabled(false);
    m_btnCancel->setEnabled(true);
    m_btnOpenFile->setEnabled(false);
    m_comboMode->setEnabled(false);

    if (m_trimEngine) m_trimEngine->deleteLater();
    m_trimEngine = new TrimEngine(m_ffmpegManager->getFFmpegPath(), this);

    connect(m_trimEngine, &TrimEngine::statusUpdated, this, [this](const QString &s) { m_statusLabel->setText(s); });
    connect(m_trimEngine, &TrimEngine::progressUpdated, this, [this](int p) { m_progressBar->setValue(p); });
    connect(m_trimEngine, &TrimEngine::trimFinished, this, [this](bool ok, const QString &err) {
        m_btnStartTrim->setEnabled(true);
        m_btnCancel->setEnabled(false);
        m_btnOpenFile->setEnabled(true);
        m_comboMode->setEnabled(true);

        if (ok) {
            m_statusLabel->setText("Trim completed successfully.");
            QMessageBox::information(this, "Success", "Video clip trimmed successfully!");
        } else {
            m_statusLabel->setText("Trim failed: " + err);
            QMessageBox::critical(this, "Error", err);
        }
    });

    auto mode = static_cast<TrimEngine::TrimMode>(m_comboMode->currentData().toInt());
    m_trimEngine->startTrim(m_loadedFilePath, outPath, m_rangeSlider->startPos(), m_rangeSlider->endPos(), mode);
}

void MainWindow::onCancelClicked() {
    if (m_trimEngine) {
        m_statusLabel->setText("Aborting trim...");
        m_trimEngine->cancel();
    }
}

void MainWindow::onCheckForUpdatesTriggered() {
    m_updateChecker->checkForUpdates(true);
}

void MainWindow::onAboutTriggered() {
    QMessageBox::about(
        this,
        "About Video Trimmer Pro",
        QString("<h3>Video Trimmer Pro v%1</h3>"
                "<p>Built with C++20 and Qt 6.</p>"
                "<p>Lossless and frame-accurate video trimming powered by FFmpeg.</p>")
                .arg(m_updateChecker->getCurrentVersion())
    );
}

void MainWindow::onUpdateAvailable(const QString &newVersion, const QString &releaseNotes, const QString &downloadUrl) {
    QString msg = QString("A new version (%1) of Video Trimmer Pro is available!\n\n%2\n\nDownload now?")
                          .arg(newVersion, releaseNotes.isEmpty() ? "Bug fixes and improvements." : releaseNotes);

    if (QMessageBox::question(this, "Update Available", msg, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    auto *dlg = new QProgressDialog("Connecting...", "Cancel", 0, 100, this);
    dlg->setWindowTitle("Downloading Update");
    dlg->setWindowModality(Qt::WindowModal);
    dlg->show();

    connect(m_updateChecker, &UpdateChecker::downloadProgress, dlg, [dlg](qint64 received, qint64 total) {
        if (total > 0) {
            dlg->setValue(static_cast<int>((received * 100) / total));
        }
    });

    connect(dlg, &QProgressDialog::canceled, m_updateChecker, &UpdateChecker::cancelDownload);
    connect(m_updateChecker, &UpdateChecker::downloadFinished, this, [this, dlg](bool ok, const QString &err) {
        dlg->close();
        dlg->deleteLater();
        if (!ok && !err.isEmpty()) QMessageBox::warning(this, "Update Error", err);
    });

    m_updateChecker->downloadAndInstallUpdate(downloadUrl, QString("VideoTrimmerPro_Setup_%1.exe").arg(newVersion));
}

void MainWindow::applyStyles() {
    setStyleSheet(R"(
        QMainWindow { background-color: #F8F9FA; }
        QLabel { color: #212529; }

        QMenuBar {
            background-color: #F8F9FA;
            color: #212529;
            border-bottom: 1px solid #CED4DA;
        }
        QMenuBar::item {
            background-color: transparent;
            color: #212529;
            padding: 4px 10px;
        }
        QMenuBar::item:selected {
            background-color: #E9ECEF;
            color: #212529;
            border-radius: 4px;
        }
        QMenu {
            background-color: #FFFFFF;
            color: #212529;
            border: 1px solid #CED4DA;
            padding: 4px;
        }
        QMenu::item {
            background-color: transparent;
            color: #212529;
            padding: 6px 24px 6px 12px;
            border-radius: 3px;
        }
        QMenu::item:selected {
            background-color: #0D6EFD;
            color: #FFFFFF;
        }

        QMessageBox, QProgressDialog { background-color: #FFFFFF; color: #212529; }
        QMessageBox QLabel, QProgressDialog QLabel { color: #212529; font-size: 13px; }

        QPushButton {
            border: 1px solid #CED4DA;
            background-color: #FFFFFF;
            color: #212529;
            padding: 6px 14px;
            border-radius: 4px;
            font-weight: 500;
        }
        QPushButton:hover { background-color: #E9ECEF; }
        QPushButton:pressed { background-color: #DEE2E6; }
        QPushButton:disabled { 
            background-color: #E9ECEF; 
            color: #ADB5BD; 
            border-color: #E9ECEF;
        }

        QComboBox, QLineEdit {
            border: 1px solid #CED4DA;
            background-color: #FFFFFF;
            color: #212529;
            padding: 6px 8px;
            border-radius: 4px;
        }

        QProgressBar {
            border: 1px solid #CED4DA;
            border-radius: 4px;
            text-align: center;
            background-color: #E9ECEF;
            color: #212529;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background-color: #0D6EFD;
            border-radius: 3px;
        }
    )");
}