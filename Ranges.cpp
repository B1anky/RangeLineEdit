#include "Ranges.h"

#include <QRegExp>

#include <iostream>
#include <cmath>

/* --- Range definitions --- */

Range::Range()
    : m_charIndexStart(0),
      m_charIndexEnd  (0),
      m_leftRange     (nullptr),
      m_rightRange    (nullptr),
      m_dirty         (false)
{
    /* NOP */
}

Range::~Range(){

    m_leftRange  = nullptr;
    m_rightRange = nullptr;

}

Range* Range::leftMostRange(){

    Range* range = this;
    while(range != nullptr && range->m_leftRange != nullptr){

        range = range->m_leftRange;

    }

    return range;

}

RangeInt* Range::leftMostRangeInt(){

    RangeInt* rangeInt  = dynamic_cast<RangeInt*>(this);
    Range*    rangeIter = this->m_leftRange;
    while(rangeIter != nullptr){

        //Production::Note: We don't break because we might find another, more left RangeInt
        if(rangeIter->rangeType() == "RangeInt"){

            rangeInt = static_cast<RangeInt*>(rangeIter);

        }

        rangeIter = rangeIter->m_leftRange;

    }

    return rangeInt;

}

bool Range::allValuesToLeftAreZero(){

    //Assume true and prove otherwise
    bool allRangeIntsToLeftAreZero(true);
    Range* rangeIter = this->m_leftRange;
    while(rangeIter != nullptr){

        //Production::Note: We don't break because we might find another, more left RangeInt
        if(rangeIter->rangeType() == "RangeInt"){

            allRangeIntsToLeftAreZero &= (static_cast<RangeInt*>(rangeIter)->m_value == 0);

        }

        rangeIter = rangeIter->m_leftRange;

    }

    return allRangeIntsToLeftAreZero;

}

bool Range::leftMostRangeCharSign(){

    //Assume true and prove otherwise
    bool positiveSign(true);
    Range* rangeIter = this->m_leftRange;
    while(rangeIter != nullptr){

        if(rangeIter->rangeType() == "RangeChar"){

            positiveSign = (static_cast<RangeChar*>(rangeIter)->m_value == static_cast<RangeChar*>(rangeIter)->m_positiveChar);

        }

        rangeIter = rangeIter->m_leftRange;

    }

    return positiveSign;

}

/* --- RangeChar definitions --- */

RangeChar::RangeChar(QChar negativeChar, QChar positiveChar)
    : Range         (),
      m_negativeChar(negativeChar),
      m_positiveChar(positiveChar),
      m_value       (m_positiveChar)
{

    /* NOP */

}

bool RangeChar::increment(int){

    bool incremented(false);
    if(m_value != m_positiveChar){

        incremented = true;
        m_value = m_positiveChar;
        m_dirty = true;

    }

    return incremented;

}

bool RangeChar::decrement(int){

    bool decremented(false);
    if(m_value != m_negativeChar){

        m_value = m_negativeChar;
        decremented = true;
        m_dirty = true;

    }

    return decremented;

}

int RangeChar::valueLength(){

    return 1;

}

int RangeChar::rangeLength(){

    return 1;

}

QString RangeChar::valueStr(){

    return QString(m_value);

}

QString RangeChar::rangeType(){

    return "RangeChar";

}

int RangeChar::divisor(){

    return 1;

}

bool RangeChar::setValueForIndex(const QString& value, int index){

    bool valueWasSet = false;

    if(value.length() == rangeLength() && index <= m_charIndexEnd){

        //Check case insensitively
        if( (value.at(0).toLower() == m_positiveChar.toLower() || value.at(0).toLower() == m_negativeChar.toLower())){

            m_value     = m_value.isUpper() ? value.at(0).toUpper() : value.at(0).toLower();
            m_dirty     = true;
            valueWasSet = true;

        }

    }

    return valueWasSet;

}

void RangeChar::setRange(QChar negativeChar, QChar positiveChar){

    m_negativeChar = negativeChar;
    m_positiveChar = positiveChar;

}

/* --- RangeStringConstant definitions --- */

RangeStringConstant::RangeStringConstant(QString stringPlaceHolder)
    : Range  (),
      m_value(stringPlaceHolder)
{

    /* NOP */

}

bool RangeStringConstant::increment(int){

    bool incremented(true);
    if(m_leftRange != nullptr){

        incremented = m_leftRange->increment(0);

    }

    return incremented;
}

bool RangeStringConstant::decrement(int){

    bool decremented(true);
    if(m_leftRange != nullptr){

        decremented = m_leftRange->decrement(0);

    }

    return decremented;

}

int RangeStringConstant::valueLength(){

    return m_value.length();

}

int RangeStringConstant::rangeLength(){

    return m_value.length();

}

QString RangeStringConstant::valueStr(){

    return m_value;

}

QString RangeStringConstant::rangeType(){

    return "RangeStringConstant";

}

int RangeStringConstant::divisor(){

    return 1;

}

bool RangeStringConstant::setValueForIndex(const QString&, int){

    return true;

}

/* --- RangeInt definitions --- */

RangeInt::RangeInt(int range, int divisor)
    : Range    (),
      m_range  (range),
      m_value  (0),
      m_divisor(divisor)
{

    /* NOP */

}

void RangeInt::setRange(int range){

    m_range = range;

}

void RangeInt::setDivisor(int divisor){

    if(divisor > 0){

        m_divisor = divisor;

    }else{

        std::cerr << QString("Error. RangeInt::setDivisor(int divisor = %0) must be called with a divisor >= 0.").arg(divisor).toStdString() << std::endl;

    }

}

bool RangeInt::increment(int index){

    //Production::Note: The index refers to the significant figure index (right to left indexing)
    //This determines if we're at the "ones", "tens", "hundreds", etc. place to determine
    //what this widget will increment by

    int valueToIncrementBy = std::pow(10, index);
    int originalValue      = m_value;

    //Incrementing a positive number (Should make the number diverge from 0 (i.e. 20 + 10 = 30)
    if(m_value > 0){

        if(m_value + valueToIncrementBy > m_range){

            if(m_leftRange != nullptr){

                //If we can increment our left Range
                if(m_leftRange->increment(0)){

                    m_value += valueToIncrementBy - m_range - 1;

                }
                //We've hit our maximum value if we couldn't increment recursively
                else{

                    m_value = m_range;

                }

            }

        }
        //Else we don't have an addition carry, so we can just normally add and not affect our neighboring left range
        else{

            m_value += valueToIncrementBy;

        }

    }
    //Incrementing a negative number (Should make the number approach 0 (i.e. -20 + 10 = -10)
    else if(m_value < 0){

        //If we increment our value above 0, but we're supposed to be negative (i.e. -05 + 10 = -55 && increment the left)
        if(m_value + valueToIncrementBy > 0){

            //If all values to our left are zeroed out, but we can flip the RangeChar (i.e. S -> N or W -> E),
            //then our addition carry will be satisfied by the RangeChar's incrementation
            bool flippingFromNegToPos = allValuesToLeftAreZero() && leftMostRange()->increment(0);

            if(flippingFromNegToPos){

                m_value = -m_value;

            }
            //If not everything to the left is zeroed out, that means we should forward to carry bit to our left range
            else if(m_leftRange != nullptr && m_leftRange->increment(0)){

                m_value = -m_range + 1 + valueToIncrementBy;

            }

        }
        //Else we can just increment because we're still going to be negative (i.e. -15 + 10 = -05)
        else{

            m_value += valueToIncrementBy;

        }

    }
    //This is a special case, depending on if our left neighbor is the RangeChar or not
    else{

        //If the whole value is positive, we can safely increment
        if(leftMostRangeCharSign() == true){

            m_value += valueToIncrementBy;

        }
        //If the whole value is negative, we need to check if everything to our left is zeroed out or not
        else{

            if(allValuesToLeftAreZero()){

                leftMostRange()->increment(0);
                m_value += valueToIncrementBy;

            }else{

                //We're actually incrementing a negative left neighboring range instead
                if(m_leftRange->increment(0)){

                    m_value =  -m_range - 1 + valueToIncrementBy;

                }

            }

        }

    }

    m_dirty = (m_value != originalValue);

    return m_dirty;

}

bool RangeInt::decrement(int index){

    //Production::Note: The index refers to the significant figure index (right to left indexing)
    //This determines if we're at the "ones", "tens", "hundreds", etc. place to determine
    //what this widget will decrement by

    int valueToDecrementBy = std::pow(10, index);
    int originalValue      = m_value;

    //Decrementing a positive number (i.e. 20 - 10 = 10)
    if(m_value > 0){

        //We're borrowing a negative from our left (if all non-zero) or the rangeChar
        if(m_value - valueToDecrementBy < 0){

            bool flippingFromPosToNeg = allValuesToLeftAreZero() && leftMostRange()->decrement(0);

            if(flippingFromPosToNeg){

                m_value = -m_value;

            }else if(m_leftRange != nullptr && m_leftRange->decrement(0)){

                m_value = m_range - (valueToDecrementBy - m_value - 1);

            }

        }else{

            m_value -= valueToDecrementBy;

        }

    }
    //Decrementing a negative number (i.e. -20 - 10 = -30)
    else if(m_value < 0){

        //If we underflow (i.e. -55 - 10 = -05 IF we can borrow from our left)
        if(m_value - valueToDecrementBy < -m_range){

            //If we can decrement the left range
            if(m_leftRange != this && m_leftRange->decrement(0)){

                //(i.e. -55 - 10 + 59 + 1 = -65 + 60 = -5)
                m_value = m_value - valueToDecrementBy + m_range + 1;

            }

        }else{

            //(i.e. -45 - 10 = -55)
            m_value -= valueToDecrementBy;

        }

    }
    //This is a special case, depending on if our left neighbor is the RangeChar or not
    else{

        if(leftMostRangeCharSign() == true){

            if(allValuesToLeftAreZero() == true){

                leftMostRange()->decrement(0);
                m_value -= valueToDecrementBy;

            }else{

                if(m_leftRange != nullptr && m_leftRange->decrement(0)){

                    m_value = -m_range - valueToDecrementBy + 1;

                }

            }

        }else{

            m_value -= valueToDecrementBy;

        }

    }

    m_dirty = (m_value != originalValue);

    return m_dirty;

}

int RangeInt::valueLength(){

    return QString::number(abs(m_value)).length();

}

int RangeInt::rangeLength(){

    return QString::number(m_range).length();

}

QString RangeInt::valueStr(){

    return QString("0").repeated(rangeLength() - valueLength()) + QString::number(abs(m_value));

}

QString RangeInt::rangeType(){

    return "RangeInt";

}

int RangeInt::divisor(){

    return m_divisor;

}

bool RangeInt::setValueForIndex(const QString& value, int index){

    bool valueWasSet = false;

    //This is to replace when the user is type numerical values at an index
    //For example, Degrees goes from [000, 180]
    //We attempt to replace the value for index string and convert the value back to an int and then verify it's within range
    if(value.length() == 1 && QRegExp("\\d").exactMatch(value)){

        int attemptedValue = valueStr().replace(index, 1, value).toInt() * (std::signbit(m_value) ? 1 : -1);

        if(fabs(attemptedValue) <= m_range){

            m_value = attemptedValue;

        }else{

            m_value = m_range * (std::signbit(m_value) ? 1 : -1);

        }

        m_dirty = true;
        valueWasSet = true;

    }

    return valueWasSet;

}
