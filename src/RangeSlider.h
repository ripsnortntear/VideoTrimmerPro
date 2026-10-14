#pragma once

#include <QWidget>
#include <QPoint>

class RangeSlider : public QWidget {
    Q_OBJECT
public:
    enum class SelectedHandle { None, Start, End, Playhead };

    explicit RangeSlider(QWidget *parent = nullptr);

    void setRange(qint64 min, qint64 max);
    void setStartPos(qint64 pos);
    void setEndPos(qint64 pos);
    void setCurrentPos(qint64 pos);

    [[nodiscard]] qint64 startPos() const { return m_startPos; }
    [[nodiscard]] qint64 endPos() const { return m_endPos; }
    [[nodiscard]] qint64 currentPos() const { return m_currentPos; }
    [[nodiscard]] SelectedHandle selectedHandle() const { return m_selectedHandle; }
    [[nodiscard]] QPoint getHandleGlobalPos(SelectedHandle handle) const;

signals:
    void rangeChanged(qint64 start, qint64 end);
    void currentPosChanged(qint64 pos);
    void handleNudged(qint64 pos);
    void handleHoverStarted(qint64 pos, QPoint globalPos, const QString &handleName);
    void handleHoverMoved(qint64 pos, QPoint globalPos);
    void handleHoverEnded();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    [[nodiscard]] int posToPixel(qint64 pos) const;
    [[nodiscard]] qint64 pixelToPos(int px) const;

    qint64 m_min{0};
    qint64 m_max{1000};
    qint64 m_startPos{0};
    qint64 m_endPos{1000};
    qint64 m_currentPos{0};

    SelectedHandle m_activeDrag{SelectedHandle::None};
    SelectedHandle m_selectedHandle{SelectedHandle::Playhead};
};