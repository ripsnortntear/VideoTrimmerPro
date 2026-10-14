#include "RangeSlider.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

RangeSlider::RangeSlider(QWidget *parent)
    : QWidget(parent) {
    setMinimumHeight(38);
    setCursor(Qt::PointingHandCursor);
}

void RangeSlider::setRange(qint64 min, qint64 max) {
    m_min = min;
    m_max = std::max(min + 1, max);
    m_startPos = m_min;
    m_endPos = m_max;
    m_currentPos = m_min;
    update();
}

void RangeSlider::setStartPos(qint64 pos) {
    m_startPos = std::clamp(pos, m_min, m_endPos - 100);
    update();
    emit rangeChanged(m_startPos, m_endPos);
}

void RangeSlider::setEndPos(qint64 pos) {
    m_endPos = std::clamp(pos, m_startPos + 100, m_max);
    update();
    emit rangeChanged(m_startPos, m_endPos);
}

void RangeSlider::setCurrentPos(qint64 pos) {
    m_currentPos = std::clamp(pos, m_min, m_max);
    update();
}

int RangeSlider::posToPixel(qint64 pos) const {
    if (m_max <= m_min) return 12;
    int w = width() - 24;
    return 12 + static_cast<int>((static_cast<double>(pos - m_min) / (m_max - m_min)) * w);
}

qint64 RangeSlider::pixelToPos(int px) const {
    int w = width() - 24;
    if (w <= 0) return m_min;
    double frac = std::clamp(static_cast<double>(px - 12) / w, 0.0, 1.0);
    return m_min + static_cast<qint64>(frac * (m_max - m_min));
}

void RangeSlider::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int trackY = height() / 2 - 3;
    int trackH = 6;
    int leftPx = posToPixel(m_startPos);
    int rightPx = posToPixel(m_endPos);
    int playPx = posToPixel(m_currentPos);

    // Track Background (Unselected)
    p.setBrush(QColor("#CED4DA"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(12, trackY, width() - 24, trackH, 3, 3);

    // Selected Active Slice
    p.setBrush(QColor("#0D6EFD"));
    p.drawRect(leftPx, trackY, std::max(2, rightPx - leftPx), trackH);

    // Start Cut Handle (Green accent)
    p.setBrush(QColor("#198754"));
    p.setPen(QColor("#146C43"));
    p.drawRoundedRect(leftPx - 5, 4, 10, height() - 8, 3, 3);

    // End Cut Handle (Red accent)
    p.setBrush(QColor("#DC3545"));
    p.setPen(QColor("#B02A37"));
    p.drawRoundedRect(rightPx - 5, 4, 10, height() - 8, 3, 3);

    // Current Playhead Position Indicator (White circle with dark rim)
    p.setBrush(Qt::white);
    p.setPen(QPen(QColor("#212529"), 2));
    p.drawEllipse(QPoint(playPx, height() / 2), 6, 6);
}

void RangeSlider::mousePressEvent(QMouseEvent *event) {
    int x = event->pos().x();
    int leftPx = posToPixel(m_startPos);
    int rightPx = posToPixel(m_endPos);
    int playPx = posToPixel(m_currentPos);

    if (std::abs(x - leftPx) <= 8) {
        m_activeHandle = ActiveHandle::Start;
    } else if (std::abs(x - rightPx) <= 8) {
        m_activeHandle = ActiveHandle::End;
    } else if (std::abs(x - playPx) <= 8) {
        m_activeHandle = ActiveHandle::Playhead;
    } else {
        // Direct click navigates playhead
        m_activeHandle = ActiveHandle::Playhead;
        m_currentPos = pixelToPos(x);
        emit currentPosChanged(m_currentPos);
        update();
    }
}

void RangeSlider::mouseMoveEvent(QMouseEvent *event) {
    if (m_activeHandle == ActiveHandle::None) return;

    qint64 pos = pixelToPos(event->pos().x());

    if (m_activeHandle == ActiveHandle::Start) {
        setStartPos(pos);
    } else if (m_activeHandle == ActiveHandle::End) {
        setEndPos(pos);
    } else if (m_activeHandle == ActiveHandle::Playhead) {
        m_currentPos = std::clamp(pos, m_min, m_max);
        emit currentPosChanged(m_currentPos);
        update();
    }
}

void RangeSlider::mouseReleaseEvent(QMouseEvent *) {
    m_activeHandle = ActiveHandle::None;
}