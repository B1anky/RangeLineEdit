#ifndef TRIANGLEPAINTEDBUTTON_H
#define TRIANGLEPAINTEDBUTTON_H

#include <QPushButton>

class QHoverEvent;

/*! class TrianglePaintedButton
 *
 * QPushButton subclass that override the paintEvent, enterEvent, and leaveEvent.
 * To be used in conjunction with a PositionalLineEdit for its SpinBox-like buttons
 * for increment and decrement operations.
 *
 */
class TrianglePaintedButton : public QPushButton{

    Q_OBJECT

public:

    /*! enum Direction
     * Denotes the direction this button will paint a triangle in during the paintEvent
     */
    enum Direction{
        UP,
        DOWN
    };

    /*
     * Value Constructor
     * @PARAM Direction direction - The direction to draw this widget's painted triangle in
     * @PARAM QWidget*  parent    - Standard Qt parenting mechanism for memory management
     */
    TrianglePaintedButton(Direction direction, QWidget* parent = nullptr);

protected slots:

    /*
     * Override of QPaintEvent.
     * Calls base QPushButton implementation first.
     * Draws a `m_direction` facing triangle based on the current width and height of this widget.
     * @PARAM QPaintEvent* paintEvent - Standard Qt paintEvent call
     */
    void paintEvent(QPaintEvent* paintEvent) override;

    /*
     * Override of QEvent::enterEvent.
     * Sets the QApplication's override cursor to a pointer.
     * @PARAM QEvent* enterEvent- Standard Qt enterEvent call
     */
    void enterEvent(QEvent* enterEvent) override;

    /*
     * Override of QEvent::leaveEvent.
     * Restores the QApplication's override cursor.
     * @PARAM QEvent* enterEvent- Standard Qt leaveEvent call
     */
    void leaveEvent(QEvent* leaveEvent) override;

protected:

    Direction m_direction;

};

#endif // TRIANGLEPAINTEDBUTTON_H
