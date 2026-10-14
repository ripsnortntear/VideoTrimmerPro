#include "RangeSlider.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <algorithm>

RangeSlider::RangeSlider(QWidget *parent)
    : QWidget(parent) {
    setMinimumHeight(44);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
}

void RangeSlider::setRange(qint64 min, qint64 max) {
    if (max <= min) max = min + 1000;
    m_min = min;
    m_max = max;
    m_startPos = m_min;
    m_endPos = m_max;
    m_currentPos = m_min;
    update();
}

void RangeSlider::setStartPos(qint64 pos) {
    m_startPos = std::clamp(pos, m_min, m_endPos - 100);
    m_selectedHandle = SelectedHandle::Start;
    update();
    emit rangeChanged(m_startPos, m_endPos);
    emit handleNudged(m_startPos);
    emit handleHoverMoved(m_startPos, getHandleGlobalPos(SelectedHandle::Start));
}

void RangeSlider::setEndPos(qint64 pos) {
    m_endPos = std::clamp(pos, m_startPos + 100, m_max);
    m_selectedHandle = SelectedHandle::End;
    update();
    emit rangeChanged(m_startPos, m_endPos);
    emit handleNudged(m_endPos);
    emit handleHoverMoved(m_endPos, getHandleGlobalPos(SelectedHandle::End));
}

void RangeSlider::setCurrentPos(qint64 pos) {
    m_currentPos = std::clamp(pos, m_min, m_max);
    update();
}

int RangeSlider::posToPixel(qint64 pos) const {
    if (m_max <= m_min) return 16;
    int w = width() - 32;
    return 16 + static_cast<int>((static_cast<double>(pos - m_min) / (m_max - m_min)) * w);
}

qint64 RangeSlider::pixelToPos(int px) const {
    int w = width() - 32;
    if (w <= 0) return m_min;
    double frac = std::clamp(static_cast<double>(px - 16) / w, 0.0, 1.0);
    return m_min + static_cast<qint64>(frac * (m_max - m_min));
}

QPoint RangeSlider::getHandleGlobalPos(SelectedHandle handle) const {
    int px = 16;
    if (handle == SelectedHandle::Start) px = posToPixel(m_startPos);
    else if (handle == SelectedHandle::End) px = posToPixel(m_endPos);
    else px = posToPixel(m_currentPos);

    return mapToGlobal(QPoint(px, 0));
}

void RangeSlider::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int trackY = height() / 2 - 3;
    int trackH = 6;
    int leftPx = posToPixel(m_startPos);
    int rightPx = posToPixel(m_endPos);
    int playPx = posToPixel(m_currentPos);

    p.setBrush(QColor("#CED4DA"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(16, trackY, width() - 32, trackH, 3, 3);

    p.setBrush(QColor("#0D6EFD"));
    p.drawRect(leftPx, trackY, std::max(2, rightPx - leftPx), trackH);

    // Green In Handle
    p.setBrush(QColor("#198754"));
    p.setPen(m_selectedHandle == SelectedHandle::Start ? QPen(QColor("#000000"), 2) : QPen(QColor("#146C43"), 1));
    p.drawRoundedRect(leftPx - 6, 6, 12, height() - 12, 3, 3);

    // Red Out Handle
    p.setBrush(QColor("#DC3545"));
    p.setPen(m_selectedHandle == SelectedHandle::End ? QPen(QColor("#000000"), 2) : QPen(QColor("#B02A37"), 1));
    p.drawRoundedRect(rightPx - 6, 6, 12, height() - 12, 3, 3);

    // Playhead
    p.setBrush(Qt::white);
    p.setPen(m_selectedHandle == SelectedHandle::Playhead ? QPen(QColor("#0D6EFD"), 2) : QPen(QColor("#212529"), 1));
    p.drawEllipse(QPoint(playPx, height() / 2), 7, 7);
}

void RangeSlider::mousePressEvent(QMouseEvent *event) {
    setFocus();
    int x = event->pos().x();
    int leftPx = posToPixel(m_startPos);
    int rightPx = posToPixel(m_endPos);
    int playPx = posToPixel(m_currentPos);

    if (std::abs(x - leftPx) <= 10) {
        m_activeDrag = SelectedHandle::Start;
        m_selectedHandle = SelectedHandle::Start;
        emit handleHoverStarted(m_startPos, getHandleGlobalPos(SelectedHandle::Start), "In Cut");
        emit handleNudged(m_startPos);
    } else if (std::abs(x - rightPx) <= 10) {
        m_activeDrag = SelectedHandle::End;
        m_selectedHandle = SelectedHandle::End;
        emit handleHoverStarted(m_endPos, getHandleGlobalPos(SelectedHandle::End), "Out Cut");
        emit handleNudged(m_endPos);
    } else if (std::abs(x - playPx) <= 10) {
        m_activeDrag = SelectedHandle::Playhead;
        m_selectedHandle = SelectedHandle::Playhead;
    } else {
        m_activeDrag = SelectedHandle::Playhead;
        m_selectedHandle = SelectedHandle::Playhead;
        m_currentPos = pixelToPos(x);
        emit currentPosChanged(m_currentPos);
    }
    update();
}

void RangeSlider::mouseMoveEvent(QMouseEvent *event) {
    if (m_activeDrag == SelectedHandle::None) return;

    qint64 pos = pixelToPos(event->pos().x());

    if (m_activeDrag == SelectedHandle::Start) {
        setStartPos(pos);
    } else if (m_activeDrag == SelectedHandle::End) {
        setEndPos(pos);
    } else if (m_activeDrag == SelectedHandle::Playhead) {
        m_currentPos = std::clamp(pos, m_min, m_max);
        emit currentPosChanged(m_currentPos);
        update();
    }
}

void RangeSlider::mouseReleaseEvent(QMouseEvent *) {
    m_activeDrag = SelectedHandle::None;
    emit handleHoverEnded();
}

void RangeSlider::keyPressEvent(QKeyEvent *event) {
    qint64 step = (event->modifiers() & Qt::ShiftModifier) ? 200 : 33;

    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        qint64 delta = (event->key() == Qt::Key_Left) ? -step : step;

        if (m_selectedHandle == SelectedHandle::Start) {
            emit handleHoverStarted(m_startPos, getHandleGlobalPos(SelectedHandle::Start), "In Cut");
            setStartPos(m_startPos + delta);
        } else if (m_selectedHandle == SelectedHandle::End) {
            emit handleHoverStarted(m_endPos, getHandleGlobalPos(SelectedHandle::End), "Out Cut");
            setEndPos(m_endPos + delta);
        } else {
            m_currentPos = std::clamp(m_currentPos + delta, m_min, m_max);
            emit currentPosChanged(m_currentPos);
            update();
        }
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}