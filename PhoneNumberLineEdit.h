#ifndef PHONENUMBERLINEEDIT_H
#define PHONENUMBERLINEEDIT_H

#include "RangeLineEdit.h"

/*! class PositionalLineEdit
 *
 * Specialized derived type of RangeLineEdit for type long long to be used for Phone numbers.
 * Supports a hyphenated phone number styled formatting for the QLineEdit. (1-800-000-0000)
 * Base class behavior bewteen the two are NOT equivalent, the following features are explicitly disabled:
 *     1. Arrow Up and Arrow Down incrementing
 *     2. Buttons representing incrementing and decrementing
 *     3. Setting Precision
 */
class PhoneNumberLineEdit : public RangeLineEdit<QString>{

    Q_OBJECT

public:

    /*
     * Value Constructor
     * @PARAM QWidget* parent                  - Standard Qt parenting mechanism for memory management
     * @PARAM bool     enableCountryCode       - Whether or not the country code should be present
     * @PARAM int      countryCodeRangeSigFigs - The Range of the country code (i.e. how many significant figures do you want)
     */
    PhoneNumberLineEdit(QWidget* parent = nullptr, bool enableCountryCode = false, int countryCodeRangeSigFigs = 0);

    /*
     * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value.
     * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
     * @PARAM QString value - The value that should be handled to populate the widget's Ranges from its specified derived type
     */
    void setValue(QString value) override;

    /*
     * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value.
     * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
     * For this type it returns its QLineEdit::text() call as a plain QString
     */
    QString value() override;

    /*
     * Convenience function to dynamically enable or modify the country code RangeInt
     * @PARAM bool enableCountryCode       - Whether or not the country code should be present
     * @PARAM int  countryCodeRangeSigFigs - The Range of the country code (i.e. how many significant figures do you want)
     */
    bool enableCountryCode(bool enableCountryCode, int countryCodeRangeSigFigs = 1);

protected:

    /*
     * Clears all Ranges properly, nulls out the memory, and clears the held list, nulls out members explicitly
     */
    void clearCurrentValidators() override;

protected slots:

    /*
     * Copies the current string value of this widget to the clipboard
     */
    void copyValueToClipboard() override;

    /*
     * Pastes a string or integer value from clipboard to populate the widget via a call to setValue(...)
     */
    void pasteValueFromClipboard() override;

    /*
     * Wraps a call to valueChanged(const QString&) signal
     */
    void valueChangedPrivate() override;

    /*
     * Connected to PositionalLineEdit::customContextMenuRequested.
     * Invoked on a right click event and spawns a custom context menu.
     * @PARAM const QPoint& pos - The position in widget coordinates that gets mapped to global coordinates to display the context menu at
     */
    void showContextMenu(const QPoint& pos) override;

    /*
     * Deprecated ability to increment for this widget
     */
    void increment() override{

        /* NOP */

    }

    /*
     * Deprecated ability to decrement for this widget
     */
    void decrement() override{

        /* NOP */

    }

    /*
     * Deprecated ability to set precision for this widget
     */
    void setPrecision(int) override{

        /* NOP */

    }

signals:

    /*
     * Type specific signal that emits a Qt-like valueChanged signal when any instance of setValue changes
     * @PARAM const QString& value - The current value() of this widget, emits when the internal value was modified
     */
    void valueChanged(const QString& value);

public:

    bool m_countryCodeEnabled;

    RangeInt*            m_countryCode;
    RangeStringConstant* m_countryCodeHyphen;
    RangeInt*            m_areaCode;
    RangeStringConstant* m_areaCodeHyphen;
    RangeInt*            m_3DigitCode;
    RangeStringConstant* m_3DigitCodeHyphen;
    RangeInt*            m_4DigitCode;

};

#endif // PHONENUMBERLINEEDIT_H
