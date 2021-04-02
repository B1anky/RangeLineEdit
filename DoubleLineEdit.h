#ifndef DOUBLELINEEDIT_H
#define DOUBLELINEEDIT_H

#include "RangeLineEdit.h"

/*! class DoubleLineEdit
 *
 * Specialized derived type of RangeLineEdit for type double to be used for any negative and positive decimal value.
 * Supports a standard double styled formatting for the QLineEdit. (i.e. -∞.xxx to +∞.xxx).
 * Allows getting and setting double values directly, in addition to copying and pasting the value from clipboard.
 */
class DoubleLineEdit : public RangeLineEdit<long double>{

    Q_OBJECT

public:

    /*
     * Value Constructor
     * @PARAM QWidget* parent   - Standard Qt parenting mechanism for memory management
     * @PARAM int      decimals - The amount of precision the user wishes to display (This does not affect stored precision)
     */
    DoubleLineEdit(QWidget* parent = nullptr, int decimals = 2);

    /*
     * Convenience function for dynamically changing the precision of the decimals.
     * Overridden to appropriately limit the m_doubleInt's maximum range such that large values don't truncate decimal precision
     * when converting the underlying value of the all Ranges back to a standard long double.
     * @PARAM int decimals - Decimal precision to be displayed. Can be set to 0 which will remove precision values, if already set.
     */
    void setPrecision(int decimals) override;

    /*
     * Calls base class implementation, and if successful will reset undisplayed precision to 0
     * @PARAM const QChar& value - The String to set at the given index
     * @PARAM int          index - The index used to lookup the held Range
     */
    bool setValueForIndex(const QChar& value, int index) override;

    /*
     * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
     * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
     * @PARAM double value - The value that should be handled to populate the widget's Ranges from its specified derived type
     */
    void setValue(long double value) override;

    /*
     * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
     * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
     * For this type it returns a decimal version of a string
     */
    long double value() override;

protected:

    /*
     * Clears all Ranges properly, nulls out the memory, and clears the held list, nulls out members explicitly
     */
    void clearCurrentValidators() override;

    /*
     * Attempts to increment the Range at the current cursor index, resets undisplayed precision to 0 if the text changed
     */
    void increment() override;

    /*
     * Attempts to decrement the Range at the current cursor index, resets undisplayed precision to 0 if the text changed
     */
    void decrement() override;

    /*
     * Helper function that ensures any changes to the value of a Range will not exceed the maximum allowable set value.
     * If the maximum is exceeded, the first-most RangeInt will be set to its range and all subsequent RangeInts will be zeroed out.
     */
    void maximumExceededFixup();

protected slots:

    /*
     * Copies the current decimal value of this widget to the clipboard
     */
    void copyValueToClipboard() override;

    /*
     * Pastes a decimal value from clipboard to populate the widget via a call to setValue(...)
     */
    void pasteValueFromClipboard() override;

    /*
     * Wraps a call to valueChanged(long double) signal
     */
    void valueChangedPrivate() override;

    /*
     * Connected to PositionalLineEdit::customContextMenuRequested.
     * Invoked on a right click event and spawns a custom context menu.
     * @PARAM const QPoint& pos - The position in widget coordinates that gets mapped to global coordinates to display the context menu at
     */
    void showContextMenu(const QPoint& pos) override;

    /*
     * Zeroes out all of the RangeInts and resets undisplayed precision to 0
     */
    void clearText() override;

signals:

    /*
     * Type specific signal that emits a Qt-like valueChanged signal when any instance of setValue changes
     * @PARAM long double value - The current value() of this widget, emits when the internal value was modified
     */
    void valueChanged(long double value);

public:

    long double m_undisplayedPrecision;

    RangeChar* m_signChar;
    RangeInt*  m_doubleInt;

};

#endif // DOUBLELINEEDIT_H
