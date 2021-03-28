#ifndef TRIANGLEPAINTEDBUTTON_H
#define TRIANGLEPAINTEDBUTTON_H

#include <QPushButton>

class QHoverEvent;

class TrianglePaintedButton : public QPushButton{

    Q_OBJECT

public:

    enum Direction{
        UP,
        DOWN
    };

    TrianglePaintedButton(Direction direction, QWidget* parent = nullptr);

    Direction m_direction;

protected slots:

    void paintEvent(QPaintEvent* paintEvent);

    void enterEvent(QEvent* enterEvent) override;

    void leaveEvent(QEvent* leaveEvent) override;

};

#endif // TRIANGLEPAINTEDBUTTON_H
