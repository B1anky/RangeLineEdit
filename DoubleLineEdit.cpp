#include "DoubleLineEdit.h"
#include "Ranges.h"
#include "TrianglePaintedButton.h"

#include <QKeyEvent>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QGuiApplication>

#include <cmath>
#include <iostream>

/* --- Public methods --- */

/*
 * Value Constructor
 */
DoubleLineEdit::DoubleLineEdit(QWidget* parent, int decimals, bool isSigned)
    : RangeLineEdit         (parent),
      m_undisplayedPrecision(0.0),
      m_signed              (isSigned),
      m_signChar            (nullptr),
      m_doubleInt           (nullptr)
{

    if(m_signed){

        m_signChar  = new RangeChar('-', '+');
        m_ranges << m_signChar;

    }

    //Production::Note: Can only truly represent from long long -> long double the following range with 16 significant figures.
    //This causes us to have to give up significant figures to the left of the decimal by a factor of 10 for each decimal more we want
    //to represent. If we were to not do this, and we made a Range close to long long max with values to the right of the decimal,
    //it may be completely disregarded due to the amount of gauranteed precision being invalidated by having >16 bits.
    m_doubleInt = new RangeInt(std::numeric_limits<long long>::max() / (100000LL * std::pow(10LL, decimals + 1) ), 1LL, true, m_signed);

    m_ranges << m_doubleInt;
    m_prevCursorPosition = 0;
    syncRangeEdges();

    m_maxAllowableValue = m_doubleInt->m_range;
    RangeLineEdit::setPrecision(decimals);

    setCursorPosition(0);
    setMinimumWidth(QFontMetrics(font()).horizontalAdvance(text()) + m_incrementButton->width());

}

/*
 * Convenience function for dynamically changing the precision of the decimals.
 * Overridden to appropriately limit the m_doubleInt's maximum range such that large values don't truncate decimal precision
 * when converting the underlying value of the all Ranges back to a standard long double.
 */
void DoubleLineEdit::setPrecision(int decimals){

    //This needs to be done prior to clearing and resyncing, but we're sure it'll succeed if the decimals >= 0
    if(decimals >= 0){

        m_doubleInt->setRange(std::numeric_limits<long long>::max() / (100000LL * std::pow(10LL, decimals + 1)));
        m_maxAllowableValue = m_doubleInt->m_range;

    }

    RangeLineEdit::setPrecision(decimals);

}

/*
 * Calls base class implementation, and if successful will reset undisplayed precision to 0
 */
bool DoubleLineEdit::setValueForIndex(const QChar& value, int index){

    bool successful(RangeLineEdit::setValueForIndex(value, index));

    if(successful){

        m_undisplayedPrecision = 0.0L;

    }

    return successful;

}

/*
 * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
 * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
 */
void DoubleLineEdit::setValue(long double value){

    const long double originalValue = DoubleLineEdit::value();

    if(m_signed){

        QChar signChar = value >= 0 ? m_signChar->m_positiveChar : m_signChar->m_negativeChar;
        m_signChar->m_value = signChar;

        if(value < 0.0){

            value = 0.0;

        }

    }

    value = std::fabs(value);

    if(value > m_maxAllowableValue){

        value = m_maxAllowableValue;

    }

    long double integer             = std::floor(value);
    long double decimalNotTruncated = 0.0L;
    long double decimal             = 0.0L;

    if(m_decimalRange != nullptr){

        //Production::Note: m_decimalRange->m_range == std::pow(10, m_decimals) - 1, so we need to divide cleanly by 10, 100, 1000, etc., not 9, 99, 999
        decimalNotTruncated = (value - integer) * (m_decimalRange->m_range);
        decimal             = std::floor(decimalNotTruncated);

    }

    m_doubleInt->m_value = integer;

    long double minimumDecimalValue(m_doubleInt->m_divisor);

    //If the user hasn't specified decimals, we need to ignore its value in determining precision loss
    if(m_decimalRange != nullptr){

        m_decimalRange->m_value = decimal;

        //Production::Note: The below episolon scales with how many decimals are being used, so it's completely dynamic
        minimumDecimalValue = (1.0L / static_cast<long double>(m_decimalRange->divisor()));

    }

    //This may or may not include decimal values
    m_undisplayedPrecision = value - sumRangeInts();

    const long double epsilonErrorAllowed(minimumDecimalValue / 10.0L);
    if(m_undisplayedPrecision > epsilonErrorAllowed){

        //Production::Note: If we found enough error to be greater than the epsilon,
        //that implies we SHOULD have incremented one more time,
        //but because doubles and floats are sometimes imprecise during arithmetic operations with certain values,
        //it erroneously floored an extra decimal value.
        //If we can increment the decimal range it should fix up any visual desync occurring from being off by 1 decimal.
        if((m_decimalRange != nullptr && m_decimalRange->increment(0)) || m_doubleInt->increment(0)){

            if(value >= 0.0L){

                //We also need to remove from the undisplayed precision the amount we were able to increment by (Brings is closer to 0)
                m_undisplayedPrecision -= minimumDecimalValue;

            }else{

                //We also need to remove from the undisplayed precision the amount we were able to increment by (Brings is closer to 0)
                m_undisplayedPrecision += minimumDecimalValue;

            }

        }

    }

    const QString originalString = text();

    syncRangeSigns();
    maximumExceededFixup();

    //If our underlying precision changed, but we can't display the visual change, the text wouldn't have changed
    //during the calls to the above two functions. As a result, we'd erroneously not emit our value changed,
    //so if we changed something the user can't see, we still want to emit the signal properly, but ensure we don't blindly
    //double emit valueChanged for no reason.
    if(originalString == text()){

       const long double newValue = DoubleLineEdit::value();

       if(originalValue != newValue){

           emit valueChanged(newValue);

       }

    }

}

/*
 * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
 * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
 * For this type it returns a decimal version of a string
 */
long double DoubleLineEdit::value(){

    //Production::Note: This is what ensures we don't lose precision when scraping the decimal value
    long double summedRangeInts(sumRangeInts());
    if(summedRangeInts >= 0.0L){

        summedRangeInts += m_undisplayedPrecision;

    }else{

        summedRangeInts -= m_undisplayedPrecision;

    }

    return summedRangeInts;

}

/* --- Protected methods --- */

/*
 * Clears all Ranges properly, nulls out the memory, and clears the held list
 */
void DoubleLineEdit::clearCurrentValidators(){

    RangeLineEdit::clearCurrentValidators();

    //Ensures everything is properly not pointing to deleted memory
    m_signChar  = nullptr;
    m_doubleInt = nullptr;

}

/*
 * Attempts to increment the Range at the current cursor index, resets undisplayed precision to 0 if the text changed
 */
void DoubleLineEdit::increment(){

    const QString prevText(text());

    RangeLineEdit::increment();

    if(prevText != text()){

        m_undisplayedPrecision = 0.0;

    }

}

/*
 * Attempts to decrement the Range at the current cursor index, resets undisplayed precision to 0 if the text changed
 */
void DoubleLineEdit::decrement(){

    const QString prevText(text());

    RangeLineEdit::decrement();

    if(prevText != text()){

        m_undisplayedPrecision = 0.0;

    }

}

/*
 * Helper function that ensures any changes to the value of a Range will not exceed the maximum allowable set value.
 * If the maximum is exceeded, the first-most RangeInt will be set to its range and all subsequent RangeInts will be zeroed out.
 */
void DoubleLineEdit::maximumExceededFixup(){

    const QString prevText(text());

    RangeLineEdit::maximumExceededFixup();

    if(prevText != text()){

        m_undisplayedPrecision = 0.0;

    }

}

/*
 * Connected to DoubleLineEdit::customContextMenuRequested.
 * Invoked on a right click event and spawns a custom context menu.
 */
void DoubleLineEdit::showContextMenu(const QPoint& pos){

    //Optionally enable / disable the paste operation depending on whether the clipboard actually holds a valid decimal string
    if(QGuiApplication::clipboard() != nullptr){

        bool canConvertToDecimal(false);
        QGuiApplication::clipboard()->text().toDouble(&canConvertToDecimal);
        m_pasteAsValueFromClipBoardAction->setEnabled(canConvertToDecimal);

    }

    RangeLineEdit::showContextMenu(pos);

}

/* --- Protected Slots ---*/

/*
 * Copies the current decimal value of this widget to the clipboard
 */
void DoubleLineEdit::copyValueToClipboard(){

    QClipboard* clipboard = QGuiApplication::clipboard();
    if(clipboard != nullptr){

        clipboard->setText(QString::number(value(), 'f', m_decimals));

    }

}

/*
 * Pastes a decimal value from clipboard to populate the widget via a call to setValue(...)
 */
void DoubleLineEdit::pasteValueFromClipboard(){

    QClipboard* clipboard = QGuiApplication::clipboard();
    if(clipboard != nullptr){

        bool isDecimal(false);
        double clipboardAsDouble = clipboard->text().toDouble(&isDecimal);
        if(isDecimal){

            setValue(clipboardAsDouble);

        }

    }

}

/*
 * Wraps a call to valueChanged(long double) signal
 */
void DoubleLineEdit::valueChangedPrivate(){

    emit valueChanged(value());

}

/*
 * Zeroes out all of the RangeInts
 */
void DoubleLineEdit::clearText(){

    RangeLineEdit::clearText();

    m_undisplayedPrecision = 0.0;

}
