#include "PhoneNumberLineEdit.h"
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
PhoneNumberLineEdit::PhoneNumberLineEdit(QWidget* parent, bool enableCountryCode, int countryCodeRangeSigFigs)
    : RangeLineEdit       (parent),
      m_countryCodeEnabled(false),
      m_countryCode       (nullptr),
      m_countryCodeHyphen (nullptr),
      m_areaCode          (nullptr),
      m_areaCodeHyphen    (nullptr),
      m_3DigitCode        (nullptr),
      m_3DigitCodeHyphen  (nullptr),
      m_4DigitCode        (nullptr)
{

    m_areaCode         = new RangeInt(999, 1, false);
    m_areaCodeHyphen   = new RangeStringConstant("-");
    m_3DigitCode       = new RangeInt(999, 1, false);
    m_3DigitCodeHyphen = new RangeStringConstant("-");
    m_4DigitCode       = new RangeInt(9999, 1, false);

    m_ranges << m_areaCode << m_areaCodeHyphen << m_3DigitCode << m_3DigitCodeHyphen << m_4DigitCode;
    m_prevCursorPosition = 0;
    syncRangeEdges();

    //Essentially the max phone number, without the country code
    m_maxAllowableValue = 9999999999;
    PhoneNumberLineEdit::enableCountryCode(enableCountryCode, countryCodeRangeSigFigs);

    setCursorPosition(0);
    setMinimumWidth(QFontMetrics(font()).horizontalAdvance(text()) + m_incrementButton->width());

    m_incrementButton->hide();
    m_decrementButton->hide();

}

/*
 * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
 * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
 */
void PhoneNumberLineEdit::setValue(QString value){

    //Sanitizes input
    bool canConvertToLongLong(false);
    value = value.remove(QRegExp("[^a-zA-Z\\d\\s]"));
    value.toLongLong(&canConvertToLongLong);

    if(canConvertToLongLong){

        const QString originalValue = PhoneNumberLineEdit::value();

        //Need to convert the value to a string, ensure it conforms to out country code rules (if applicable) and then
        //split the value based on assumptions of how phone numbers are represented
        QString valueStr = value;

        //(i.e ccc-aaa-333-4444)
        const int expectedMinimumLength = 10;
        int expectedTotalLength = expectedMinimumLength;
        if(m_countryCode != nullptr){

            //(i.e. 3 sig figs "999")
            expectedTotalLength += m_countryCode->rangeLength();

        }

        //If they copy a country code, but this widget doesn't support country codes, then truncate it
        if(valueStr.length() > expectedTotalLength){

            valueStr = valueStr.right(expectedTotalLength);

        }

        //So long as all preconditions are met, we can attempt to split the string up and assign the values to each Range properly
        if(valueStr.length() <= expectedTotalLength){

            //If the user set the value with something missing sig figs, we assume we zero out the left-most missing
            //values to make the string the appropriate length
            valueStr = valueStr.prepend(QString("0").repeated(expectedTotalLength - valueStr.length()));

            int curInd = 0;

            //Let's go from left to right, starting with the country code (if applicable)
            if(m_countryCode != nullptr){

                m_countryCode->setValue(valueStr.mid(curInd, m_countryCode->rangeLength()).toLongLong());
                curInd += m_countryCode->rangeLength();

            }

            //Area code
            m_areaCode->setValue(valueStr.mid(curInd, m_areaCode->rangeLength()).toLongLong());
            curInd += m_areaCode->rangeLength();

            //3 digit code
            m_3DigitCode->setValue(valueStr.mid(curInd, m_3DigitCode->rangeLength()).toLongLong());
            curInd += m_3DigitCode->rangeLength();

            //4 digit code
            m_4DigitCode->setValue(valueStr.mid(curInd, m_4DigitCode->rangeLength()).toLongLong());

        }

        const QString originalString = text();
        scrapeDirtiedRanges(true);

        //If our underlying precision changed, but we can't display the visual change, the text wouldn't have changed
        //during the calls to the above two functions. As a result, we'd erroneously not emit our value changed,
        //so if we changed something the user can't see, we still want to emit the signal properly, but ensure we don't blindly
        //double emit valueChanged for no reason.
        if(originalString == text()){

           const QString newValue = PhoneNumberLineEdit::value();

           if(originalValue != newValue){

               emit valueChanged(newValue);

           }

        }

    }

}

/*
 * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
 * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
 * For this type it returns a decimal version of a string
 */
QString PhoneNumberLineEdit::value(){

    return text();

}

/*
 * Convenience function to dynamically enable or modify the country code RangeInt
 */
bool PhoneNumberLineEdit::enableCountryCode(bool enableCountryCode, int countryCodeRangeSigFigs){

    bool successful(true);

    //Only valid if they want to enable a country code with valid sig figs OR they want to turn off country code (ignore sig figs)
    if( (enableCountryCode == true && countryCodeRangeSigFigs > 0) || (enableCountryCode == false)){

        m_countryCodeEnabled = enableCountryCode;

        //We have to prepend to the m_ranges
        if(enableCountryCode == true){

           //Only allocate memory if we have to
           if(m_countryCode == nullptr && m_countryCodeHyphen == nullptr){

                m_countryCode       = new RangeInt(std::pow(10LL, countryCodeRangeSigFigs) - 1LL, 1LL, false);
                m_countryCodeHyphen = new RangeStringConstant("-");

                m_ranges.prepend(m_countryCodeHyphen);
                m_ranges.prepend(m_countryCode);

           }else if(m_countryCode != nullptr && m_countryCodeHyphen != nullptr){

               //In case the user was just changing the sig figs, but the country code was already enabled
               m_countryCode->setRange(std::pow(10LL, countryCodeRangeSigFigs) - 1LL);

           }

        }
        //We have to remove from the front of ranges and delete the memory
        else if(enableCountryCode == false && m_countryCode != nullptr && m_countryCodeHyphen != nullptr){

            //Pop the country code off
            m_ranges.pop_front();

            //Pop the country code hyphen off
            m_ranges.pop_front();

            delete m_countryCode;
            delete m_countryCodeHyphen;

            m_countryCode       = nullptr;
            m_countryCodeHyphen = nullptr;

        }

        clearText();
        syncRangeEdges();

    }else{

        std::cerr << QString("Error. PhoneNumberLineEdit::enableCountryCode(bool enableCountryCode = %0, int countryCodeRangeSigFigs = %1) must be called with a range > 0.").arg(enableCountryCode).arg(countryCodeRangeSigFigs).toStdString() << std::endl;
        successful = false;

    }

    return successful;

}

/* --- Protected methods --- */

/*
 * Clears all Ranges properly, nulls out the memory, and clears the held list
 */
void PhoneNumberLineEdit::clearCurrentValidators(){

    RangeLineEdit::clearCurrentValidators();

    //Ensures everything is properly not pointing to deleted memory
    m_countryCode       = nullptr;
    m_countryCodeHyphen = nullptr;
    m_areaCode          = nullptr;
    m_areaCodeHyphen    = nullptr;
    m_3DigitCode        = nullptr;
    m_3DigitCodeHyphen  = nullptr;
    m_4DigitCode        = nullptr;

}

/*
 * Connected to PhoneNumberLineEdit::customContextMenuRequested.
 * Invoked on a right click event and spawns a custom context menu.
 */
void PhoneNumberLineEdit::showContextMenu(const QPoint& pos){

    //Optionally enable / disable the paste operation depending on whether the clipboard actually holds a valid phone number
    if(QGuiApplication::clipboard() != nullptr){

        bool canConvertToLongLong(false);
        QString clipboardText = QGuiApplication::clipboard()->text();
        clipboardText = clipboardText.remove(QRegExp("[^a-zA-Z\\d\\s]"));
        clipboardText.toLongLong(&canConvertToLongLong);
        m_pasteAsValueFromClipBoardAction->setEnabled(canConvertToLongLong);

    }

    RangeLineEdit::showContextMenu(pos);

}

/* --- Protected Slots ---*/

/*
 * Copies the current decimal value of this widget to the clipboard
 */
void PhoneNumberLineEdit::copyValueToClipboard(){

    QClipboard* clipboard = QGuiApplication::clipboard();
    if(clipboard != nullptr){

        clipboard->setText(value());

    }

}

/*
 * Pastes a decimal value from clipboard to populate the widget via a call to setValue(...)
 */
void PhoneNumberLineEdit::pasteValueFromClipboard(){

    QClipboard* clipboard = QGuiApplication::clipboard();
    if(clipboard != nullptr){

        bool canConvertToLongLong(false);
        QString clipboardText = QGuiApplication::clipboard()->text();
        clipboardText = clipboardText.remove(QRegExp("[^a-zA-Z\\d\\s]"));
        clipboardText.toLongLong(&canConvertToLongLong);
        if(canConvertToLongLong){

            setValue(clipboardText);

        }

    }

}

/*
 * Wraps a call to valueChanged(long double) signal
 */
void PhoneNumberLineEdit::valueChangedPrivate(){

    emit valueChanged(value());

}
