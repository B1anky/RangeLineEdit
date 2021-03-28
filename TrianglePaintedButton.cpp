#include "TrianglePaintedButton.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

TrianglePaintedButton::TrianglePaintedButton(Direction direction, QWidget* parent)
    : QPushButton(parent),
      m_direction(direction)
{

}

void TrianglePaintedButton::paintEvent(QPaintEvent* paintEvent)
{


    QPushButton::paintEvent(paintEvent);

    QPainter painter(this);
    QPainterPath path;
    QPolygon trianglePolygon;
    if(m_direction == Direction::UP){
        //Draw from bottom left, to middle top, to bottom right
        trianglePolygon << QPoint(this->width() * 0.20, this->height() * 0.80);
        trianglePolygon << QPoint(this->width() / 2.0,  this->height() * 0.10);
        trianglePolygon << QPoint(this->width() * 0.80, this->height() * 0.80);
    }else{
        //Draw from top left, to middle bottom, to top right
        trianglePolygon << QPoint(this->width() * 0.20, this->height() * 0.20);
        trianglePolygon << QPoint(this->width() / 2.0,  this->height() * 0.80);
        trianglePolygon << QPoint(this->width() * 0.80, this->height() * 0.20);
    }
    path.addPolygon(trianglePolygon);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillPath(path, Qt::black);


}
