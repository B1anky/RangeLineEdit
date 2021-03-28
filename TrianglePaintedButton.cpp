#include "TrianglePaintedButton.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QHoverEvent>
#include <QGuiApplication>

TrianglePaintedButton::TrianglePaintedButton(Direction direction, QWidget* parent)
    : QPushButton(parent),
      m_direction(direction)
{

    //This enables the hoverEvent propogation from occurring when even not in focus
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);

}

void TrianglePaintedButton::paintEvent(QPaintEvent* paintEvent){

    QPushButton::paintEvent(paintEvent);

    QPainter     painter(this);
    QPainterPath path;
    QPolygon     trianglePolygon;

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

void TrianglePaintedButton::enterEvent(QEvent* enterEvent){

    //Since we're embedding these buttons directly into the PositionalLineEdit absolutely, the cursor
    //thinks we should be the `I` shape when hovering, not the pointer shape when we're hovering, sop this fixes that.
    QGuiApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    QPushButton::enterEvent(enterEvent);

}

void TrianglePaintedButton::leaveEvent(QEvent* leaveEvent){

    QGuiApplication::restoreOverrideCursor();
    QPushButton::leaveEvent(leaveEvent);

}
