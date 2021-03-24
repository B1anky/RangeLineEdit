#include "PositionalLineEdits.h"
#include <QKeyEvent>
#include <cmath>

#include <QDebug>

PositionalLineEdits::PositionalLineEdits(Type type, int decimals, QWidget* parent)
    : QLineEdit     (parent),
      m_ranges      ({}),
      m_decimals    (-1),
      m_decimalRange(nullptr)
{
    setType(type);
    setDecimals(decimals);
}

void PositionalLineEdits::setType(PositionalLineEdits::Type type){

    //Lazily evaluate so we don't waste run time if it should really be a NOP
    if(type != m_type){

        clearCurrentValidators();

        if(type == Type::LATITUDE){

            setLatitudeRangeValidators();

        }else{

            setLongitudeRangeValidators();

        }

        //If we have decimals, then we make a millisecond range where each decimal adds on another factor of 10
        setDecimals(m_decimals);

    }

}

void PositionalLineEdits::setDecimals(int decimals)
{

    if(m_decimals != decimals && decimals >= 0){

        m_decimals = decimals;
        int currentCursorPos = this->cursorPosition();
        QString curText = text();

        //Generally this occurs if we're setting our type for the first time or changing our type dynamically
        if(m_decimalRange == nullptr){

            m_decimalRange = new RangeInt(0, 0, m_decimals);
            if(m_ranges.isEmpty() == false){

                m_decimalRange->m_leftRange   = m_ranges.last();
                m_ranges.last()->m_rightRange = m_decimalRange;

                //Offset by 3 since there's a ''. in between seconds and milliseconds
                m_decimalRange->m_charIndexStart = m_ranges.last()->m_charIndexEnd + 3;
                m_decimalRange->m_charIndexEnd   = m_decimalRange->m_charIndexStart + QString::number(m_decimalRange->m_range).length();

            }

            m_ranges << m_decimalRange;

            curText = curText.split(".").first();
            curText += "." + QString("0").repeated(m_decimals - QString::number(m_decimalRange->m_range).length()) + QString::number(m_decimalRange->m_value);

        }
        //Generally this will only occur when the user calls setDecimals manually
        else{

            m_decimalRange->m_range = std::pow(10, m_decimals) - 1;
            int endIndexOffset = QString::number(m_decimalRange->m_range).length();
            m_decimalRange->m_charIndexEnd = m_decimalRange->m_charIndexStart + endIndexOffset;

            QString curText = text();
            curText = curText.split(".").first();
            curText += "." + QString("0").repeated(m_decimals - endIndexOffset) + QString::number(m_decimalRange->m_value);

        }

        setText(curText);
        setCursorPosition(currentCursorPos);

    }

}

Range* PositionalLineEdits::getRangeForIndex(int index){

    Range* rangeForIndex(nullptr);
    foreach(Range* range, m_ranges){

        if(index >= range->m_charIndexStart && index <= range->m_charIndexEnd){

            rangeForIndex = range;
            break;

        }

    }

    return rangeForIndex;

}

void PositionalLineEdits::clearCurrentValidators()
{

    foreach(Range* range, m_ranges){

        if(dynamic_cast<RangeInt*>(range)){

            delete dynamic_cast<RangeInt*>(range);

        }else if(dynamic_cast<RangeChar*>(range)){

            delete dynamic_cast<RangeChar*>(range);

        }else{

            delete range;

        }

        range = nullptr;

    }

    m_ranges.clear();

    //Should've been deleted in the above loop, but let's make sure anyway
    delete m_decimalRange;
    m_decimalRange = nullptr;

}

void PositionalLineEdits::setLatitudeRangeValidators()
{

    RangeChar* degreeChar = new RangeChar(0, 0, 'W', 'E');
    RangeInt*  degreeInt  = new RangeInt(1, 3, 180);
    //Then we insert U+00B0 (Takes up index 4)
    RangeInt*  minuteInt  = new RangeInt(5, 6, 60);
    //Then we insert ' (Takes up index 7)
    RangeInt*  secondsInt = new RangeInt(8, 9, 60);
    //Then we insert '' (Takes up index 10)

    //Now set all of their respective left and right ranges
    degreeChar->m_rightRange = degreeInt;
    degreeInt->m_leftRange   = degreeChar;

    degreeInt->m_rightRange = minuteInt;
    minuteInt->m_leftRange  = degreeInt;

    minuteInt->m_rightRange = secondsInt;
    secondsInt->m_leftRange = minuteInt;

    m_ranges << degreeChar << degreeInt << minuteInt << secondsInt;

    setText("E000°00'00''");

}

void PositionalLineEdits::setLongitudeRangeValidators()
{

    RangeChar* degreeChar = new RangeChar(0, 0, 'S', 'N');
    RangeInt*  degreeInt  = new RangeInt(1, 2, 90);
    //Then we insert U+00B0 (Takes up index 3)
    RangeInt*  minuteInt  = new RangeInt(4, 5, 60);
    //Then we insert ' (Takes up index 6)
    RangeInt*  secondsInt = new RangeInt(7, 8, 60);
    //Then we insert '' (Takes up index 9)

    //Now set all of their respective left and right ranges
    degreeChar->m_rightRange = degreeInt;
    degreeInt->m_leftRange   = degreeChar;

    degreeInt->m_rightRange = minuteInt;
    minuteInt->m_leftRange  = degreeInt;

    minuteInt->m_rightRange = secondsInt;
    secondsInt->m_leftRange = minuteInt;

    m_ranges << degreeChar << degreeInt << minuteInt << secondsInt;

    setText("N00°00'00''");

}

void PositionalLineEdits::replaceTextForRange(Range* range){

    if(range->m_dirty){

        //Set the string based on the value, padded by 0s where the value doesn't reach the length
        int length = range->m_charIndexEnd - range->m_charIndexStart + 1;

        RangeInt* asRangeInt = dynamic_cast<RangeInt*>(range);
        QString curText      = this->text();
        if(Q_LIKELY(asRangeInt)){

            QString intAsStr = QString::number(asRangeInt->m_value);
            intAsStr = QString("0").repeated(length - intAsStr.length()) + intAsStr;
            curText = curText.replace(range->m_charIndexStart, length, intAsStr);

        }else{

            RangeChar* asRangeChar = dynamic_cast<RangeChar*>(range);
            curText = curText.replace(range->m_charIndexStart, range->m_charIndexEnd + 1, asRangeChar->m_value);

        }

        range->m_dirty = false;
        setText(curText);

    }

}

void PositionalLineEdits::scrapeDirtiedRanges(){

    foreach(Range* range, m_ranges){

        replaceTextForRange(range);

    }

}

void PositionalLineEdits::keyPressEvent(QKeyEvent* keyEvent)
{

    int key = keyEvent->key();

    if(key == Qt::Key_Up || key == Qt::Key_Down){

        int focusIndex = this->cursorPosition();
        //Check if this the position that belongs to a Range
        Range* range = getRangeForIndex(focusIndex);

        if(range != nullptr){

            key == Qt::Key_Up ? range->increment() : range->decrement();

            scrapeDirtiedRanges();
            setCursorPosition(focusIndex);

        }

    }else if(key == Qt::Key_Left || key == Qt::Key_Right){

        QLineEdit::keyPressEvent(keyEvent);

    }else if(key == Qt::Key_P){

        qDebug() << "Cursor position of Text Edit:" << this->cursorPosition();

        foreach(Range* range, m_ranges){

            QString debugStr = QString("Start: %0, "
                                       "  End: %1, ").arg(range->m_charIndexStart).arg(range->m_charIndexEnd);

            RangeInt* isRangeInt = dynamic_cast<RangeInt*>(range);
            if(Q_LIKELY(isRangeInt)){

                debugStr += QString("Value: %0").arg(isRangeInt->m_value);

            }else{

                RangeChar* isRangeChar = dynamic_cast<RangeChar*>(range);
                debugStr += QString("Value: %0").arg(isRangeChar->m_value);

            }

            qDebug() << debugStr;

        }

    }

}

RangeInt::RangeInt(int charIndexStart, int charIndexEnd, int range)
    : Range(charIndexStart, charIndexEnd),
      m_range(range),
      m_value(0)
{
    //NOP
}

void RangeInt::setRange(int range)
{
    m_range = range;
}

bool RangeInt::increment(){

    bool incremented(true);
    m_dirty = true;

    if(m_value + 1 >= m_range){

        if(m_leftRange != nullptr){

            if(m_leftRange->increment()){

                m_value = 0;
                m_dirty = true;

            }else{

                incremented = false;
                m_dirty = false;

            }

        }

    }else{
        ++m_value;
    }

    return incremented;

}

bool RangeInt::decrement(){

    bool decremented(true);
    m_dirty = true;

    if(m_value - 1 < 0){

        if(m_leftRange != nullptr){

            if(m_leftRange->decrement()){

                m_value = m_range - 1;

            }else{

                decremented = false;
                m_dirty = false;

            }

        }

    }else{

        --m_value;

    }

    return decremented;

}

bool RangeChar::increment()
{
    bool incremented(false);
    if(m_value != m_positiveChar){
        m_value = m_positiveChar;
        incremented = true;
        m_dirty = true;
    }

    return incremented;
}

bool RangeChar::decrement()
{
    bool decremented(false);
    if(m_value != m_negativeChar){
        m_value = m_negativeChar;
        decremented = true;
        m_dirty = true;
    }

    return decremented;
}

void RangeChar::setRange(QChar negativeChar, QChar positiveChar)
{
    m_negativeChar = negativeChar;
    m_positiveChar = positiveChar;
}

RangeChar::RangeChar(int charIndexStart, int charIndexEnd, QChar negativeChar, QChar positiveChar)
    : Range(charIndexStart, charIndexEnd),
      m_negativeChar(negativeChar),
      m_positiveChar(positiveChar),
      m_value       (m_positiveChar)
{
    //NOP
}

Range::Range(int charIndexStart, int charIndexEnd)
    : m_charIndexStart(charIndexStart),
      m_charIndexEnd  (charIndexEnd),
      m_leftRange     (nullptr),
      m_rightRange    (nullptr),
      m_dirty         (false)
{
    //NOP
}
