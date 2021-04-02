#ifndef POSITIONALLINEEDIT_H
#define POSITIONALLINEEDIT_H

#include "RangeLineEdit.h"

/*! class PositionalLineEdit
 *
 * Specialized derived type of RangeLineEdit for type double to be used for Latitude and Longitude values.
 * Supports a DMS (Degree, Minute, Second) styled formatting for the QLineEdit. (i.e. N90°00'00.00'' or E180°00'00.00'')
 * Base class behavior bewteen the two are equivalent, the only difference,
 * for the most part between Latitude and Longitude are their degree sign's characters and their degree's ranges.
 */
class PositionalLineEdit : public RangeLineEdit<double>{

    Q_OBJECT

public:

    /*
     * Value Constructor
     * @PARAM QWidget* parent - Standard Qt parenting mechanism for memory management
     */
    PositionalLineEdit(QWidget* parent = nullptr);

    /*
     * Calls base class implementation, and if successful will reset undisplayed precision to 0
     * @PARAM const QChar& value - The String to set at the given index
     * @PARAM int          index - The index used to lookup the held Range
     */
    bool setValueForIndex(const QChar& value, int index) override;

    /*
     * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
     * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
     * @PARAM long double value - The value that should be handled to populate the widget's Ranges from its specified derived type
     */
    void setValue(double value) override;

    /*
     * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
     * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
     * For this type it returns a decimal version of a string
     */
    double value() override;

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
    void maximumExceededFixup() override;

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
     * Wraps a call to valueChanged(double) signal
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
     * @PARAM double value - The current value() of this widget, emits when the internal value was modified
     */
    void valueChanged(double value);

public:

    double m_undisplayedPrecision;

    RangeChar*           m_degreeChar;
    RangeInt*            m_degreeInt;
    RangeStringConstant* m_degreeSymbol;
    RangeInt*            m_minuteInt;
    RangeStringConstant* m_minuteSymbol;
    RangeInt*            m_secondsInt;
    RangeStringConstant* m_secondSymbol;

};

#endif // POSITIONALLINEEDIT_H
