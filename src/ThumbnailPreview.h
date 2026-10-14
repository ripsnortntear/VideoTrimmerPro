#pragma once

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>

class ThumbnailPreview : public QWidget {
    Q_OBJECT
public:
    explicit ThumbnailPreview(QWidget *parent = nullptr)
        : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint) {
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TranslucentBackground);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);

        m_imgLabel = new QLabel(this);
        m_imgLabel->setFixedSize(160, 90);
        m_imgLabel->setStyleSheet("background-color: #000000; border-radius: 4px; border: 1px solid #333;");
        m_imgLabel->setScaledContents(true);

        m_timeLabel = new QLabel(this);
        m_timeLabel->setAlignment(Qt::AlignCenter);
        m_timeLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 11px; background-color: rgba(0, 0, 0, 180); border-radius: 3px; padding: 2px;");

        layout->addWidget(m_imgLabel);
        layout->addWidget(m_timeLabel);

        setStyleSheet("background-color: rgba(33, 37, 41, 230); border-radius: 6px; border: 1px solid #495057;");
    }

    void updatePreview(const QPixmap &pix, const QString &timestamp) {
        if (!pix.isNull()) {
            m_imgLabel->setPixmap(pix);
        }
        m_timeLabel->setText(timestamp);
    }

private:
    QLabel *m_imgLabel{nullptr};
    QLabel *m_timeLabel{nullptr};
};