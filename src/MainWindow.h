#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QLabel>
#include <QComboBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>

#include "RangeSlider.h"
#include "FFmpegManager.h"
#include "ThumbnailPreview.h"
#include "TrimEngine.h"
#include "UpdateChecker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onOpenFileClicked();
    void onBrowseOutputClicked();
    void onPlayPauseClicked();
    void onMediaPositionChanged(qint64 position);
    void onMediaDurationChanged(qint64 duration);
    void onSetStartToCurrent();
    void onSetEndToCurrent();
    void onRangeChanged(qint64 start, qint64 end);
    void onStartTrimClicked();
    void onCancelClicked();
    void onHandleHoverStarted(qint64 pos, QPoint globalPos, const QString &name);
    void onHandleHoverMoved(qint64 pos, QPoint globalPos);
    void onHandleHoverEnded();
    void fetchThumbnail(qint64 posMs);
    void onCheckForUpdatesTriggered();
    void onAboutTriggered();
    void onUpdateAvailable(const QString &newVersion, const QString &releaseNotes, const QString &downloadUrl);

private:
    void setupUi();
    void setupMenuBar();
    void applyStyles();
    void loadVideo(const QString &path);
    void updateTimestampLabels();
    void verifyOrAcquireFFmpeg(); // <-- Declared here

    ThumbnailPreview *m_thumbPreview{nullptr};
    QProcess *m_thumbProcess{nullptr};
    qint64 m_pendingThumbMs{-1};
    QMediaPlayer *m_player{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    QVideoWidget *m_videoWidget{nullptr};
    FFmpegManager *m_ffmpegManager{nullptr};
    TrimEngine *m_trimEngine{nullptr};
    UpdateChecker *m_updateChecker{nullptr};

    QString m_loadedFilePath;

    QPushButton *m_btnOpenFile{nullptr};
    QPushButton *m_btnPlayPause{nullptr};
    RangeSlider *m_rangeSlider{nullptr};

    QPushButton *m_btnSetStart{nullptr};
    QPushButton *m_btnSetEnd{nullptr};
    QLabel *m_lblTrimPoints{nullptr};
    QLabel *m_lblDurationInfo{nullptr};

    QLineEdit *m_editOutputPath{nullptr};
    QPushButton *m_btnBrowseOutput{nullptr};

    QComboBox *m_comboMode{nullptr};
    QProgressBar *m_progressBar{nullptr};
    QPushButton *m_btnStartTrim{nullptr};
    QPushButton *m_btnCancel{nullptr};
    QLabel *m_statusLabel{nullptr};
};