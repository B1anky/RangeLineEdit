#include "PositionalLineEdit.h"
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
PositionalLineEdit::PositionalLineEdit(QWidget* parent)
    : RangeLineEdit         (parent),
      m_undisplayedPrecision(0.0),
      m_degreeChar          (nullptr),
      m_degreeInt           (nullptr),
      m_degreeSymbol        (nullptr),
      m_minuteInt           (nullptr),
      m_minuteSymbol        (nullptr),
      m_secondsInt          (nullptr),
      m_secondSymbol        (nullptr)
{

    /* NOP */

}

/*
 * Calls base class implementation, and if successful will reset undisplayed precision to 0
 */
bool PositionalLineEdit::setValueForIndex(const QChar& value, int index){

    bool successful(RangeLineEdit::setValueForIndex(value, index));

    if(successful){

        m_undisplayedPrecision = 0.0;

    }

    return successful;

}

/*
 * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
 * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
 */
void PositionalLineEdit::setValue(double value){

    const double originalValue = PositionalLineEdit::value();

    QChar signChar = value >= 0 ? m_degreeChar->m_positiveChar : m_degreeChar->m_negativeChar;
    m_degreeChar->m_value = signChar;

    value = std::fabs(value);

    if(value > m_maxAllowableValue){

        value = m_maxAllowableValue;

    }

    double degrees                    = std::floor(value);
    double minutesNotTruncated        = (value - degrees) * 60.0;
    double minutes                    = std::floor(minutesNotTruncated);
    double secondsNotTruncated        = (minutesNotTruncated - minutes) * 60.0;
    double seconds                    = std::floor(secondsNotTruncated);
    double decimalSecondsNotTruncated = 0.0;
    double decimalSeconds             = 0.0;

    if(m_decimalRange != nullptr){

        //Production::Note: m_decimalRange->m_range == std::pow(10LL, m_decimals) - 1LL, so we need to divide cleanly by 10, 100, 1000, etc., not 9, 99, 999
        decimalSecondsNotTruncated = (secondsNotTruncated - seconds) * (m_decimalRange->m_range + 1.0L);
        decimalSeconds             = std::floor(decimalSecondsNotTruncated);

    }

    minutes        = std::floor(minutes);
    seconds        = std::floor(seconds);
    decimalSeconds = std::floor(decimalSeconds);

    m_degreeInt->m_value  = degrees;
    m_minuteInt->m_value  = minutes;
    m_secondsInt->m_value = seconds;

    long double minimumDecimalValue(1.0L / m_secondsInt->divisor());

    //If the user hasn't specified decimals, we need to ignore its value in determining precision loss
    if(m_decimalRange != nullptr){

        m_decimalRange->m_value = decimalSeconds;

        //Production::Note: The below episolon scales with how many decimals are being used, so it's completely dynamic
        minimumDecimalValue = (1.0L / (m_secondsInt->divisor() * (m_decimalRange->m_range + 1.0)));

    }

    //This may or may not include decimal values
    m_undisplayedPrecision = value - sumRangeInts();

    const double epsilonErrorAllowed(minimumDecimalValue / 10.0);
    if(m_undisplayedPrecision > epsilonErrorAllowed){

        //Production::Note: If we found enough error to be greater than the epsilon,
        //that implies we SHOULD have incremented one more time,
        //but because doubles and floats are sometimes imprecise during arithmetic operations with certain values,
        //it erroneously floored an extra decimal value.
        //If we can increment the decimal range it should fix up any visual desync occurring from being off by 1 decimal.
        if((m_decimalRange != nullptr && m_decimalRange->increment(0)) || m_secondsInt->increment(0)){

            if(value >= 0){

                //We also need to remove from the undisplayed precision the amount we were able to increment by (Brings is closer to 0)
                m_undisplayedPrecision -= minimumDecimalValue;

            }else{

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

       const double newValue = PositionalLineEdit::value();

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
double PositionalLineEdit::value(){

    //Production::Note: This is what ensures we don't lose precision when scraping the decimal value
    double summedRangeInts(sumRangeInts());
    if(summedRangeInts >= 0.0){

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
void PositionalLineEdit::clearCurrentValidators(){

    RangeLineEdit::clearCurrentValidators();

    //Ensures everything is properly not pointing to deleted memory
    m_degreeChar   = nullptr;
    m_degreeInt    = nullptr;
    m_degreeSymbol = nullptr;
    m_minuteInt    = nullptr;
    m_minuteSymbol = nullptr;
    m_secondsInt   = nullptr;
    m_secondSymbol = nullptr;

}

/*
 * Attempts to increment the Range at the current cursor index, resets undisplayed precision to 0 if the text changed
 */
void PositionalLineEdit::increment(){

    const QString prevText(text());

    RangeLineEdit::increment();

    if(prevText != text()){

        m_undisplayedPrecision = 0.0;

    }

}

/*
 * Attempts to decrement the Range at the current cursor index, resets undisplayed precision to 0 if the text changed
 */
void PositionalLineEdit::decrement(){

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
void PositionalLineEdit::maximumExceededFixup(){

    const QString prevText(text());

    RangeLineEdit::maximumExceededFixup();

    if(prevText != text()){

        m_undisplayedPrecision = 0.0;

    }

}

/*
 * Connected to PositionalLineEdit::customContextMenuRequested.
 * Invoked on a right click event and spawns a custom context menu.
 */
void PositionalLineEdit::showContextMenu(const QPoint& pos){

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
void PositionalLineEdit::copyValueToClipboard(){

    QClipboard* clipboard = QGuiApplication::clipboard();
    if(clipboard != nullptr){

        clipboard->setText(QString::number(value(), 'f', m_decimals));

    }

}

/*
 * Pastes a decimal value from clipboard to populate the widget via a call to setValue(...)
 */
void PositionalLineEdit::pasteValueFromClipboard(){

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
 * Wraps a call to valueChanged(double) signal
 */
void PositionalLineEdit::valueChangedPrivate(){

    emit valueChanged(value());

}

/*
 * Zeroes out all of the RangeInts
 */
void PositionalLineEdit::clearText(){

    RangeLineEdit::clearText();

    m_undisplayedPrecision = 0.0;

}
