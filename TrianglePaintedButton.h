#ifndef TRIANGLEPAINTEDBUTTON_H
#define TRIANGLEPAINTEDBUTTON_H

#include <QPushButton>

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



};

#endif // TRIANGLEPAINTEDBUTTON_H
