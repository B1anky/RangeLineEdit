#include "Ranges.h"

#include <QRegExp>

#include <iostream>
#include <cmath>

/* --- Range definitions --- */

/*
 * Default Constructor
 */
Range::Range()
    : m_charIndexStart(0),
      m_charIndexEnd  (0),
      m_leftRange     (nullptr),
      m_rightRange    (nullptr),
      m_dirty         (false)
{

    /* NOP */

}

/*
 * Destructor
 * Nulls out left and range ranges
 */
Range::~Range(){

    m_leftRange  = nullptr;
    m_rightRange = nullptr;

}

/*
 * Attempts to return whichever Range type is the head of the Ranges.
 * Will return nullptr if this is called on the head.
 */
Range* Range::leftMostRange(){

    Range* range = m_leftRange;
    while(range != nullptr && range->m_leftRange != nullptr){

        range = range->m_leftRange;

    }

    return range;

}

/*
 * Attempts to return whichever RangeInt type is the most significant value of all Ranges.
 * Will return itself if it's already the left-most RangeInt.
 * Will return nullptr if this is not a RangeInt and there are no RangeInt types to its left.
 */
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

/*
 * Checks all RangeInt types to the left of `this` for a non-zero RangeInt.
 * Used in conjunction with RangeInt's increment and decrement specifically for edge case behavior.
 */
bool Range::allValuesToLeftAreZero(){

    //Assume true and prove otherwise
    bool allRangeIntsToLeftAreZero(true);
    Range* rangeIter = this->m_leftRange;
    while(rangeIter != nullptr){

        //Production::Note: We don't break because we might find another, more left RangeInt
        if(rangeIter->rangeType() == "RangeInt"){

            allRangeIntsToLeftAreZero &= (static_cast<RangeInt*>(rangeIter)->m_value == 0LL);

        }

        rangeIter = rangeIter->m_leftRange;

    }

    return allRangeIntsToLeftAreZero;

}

/*
 * Returns if the left-most RangeChar is currently in its positive (true) or negative (false) state.
 * Used in conjunction with RangeInt's increment and decrement specifically for edge case behavior.
 */
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

/*
 * Value Constructor
 */
RangeChar::RangeChar(QChar negativeChar, QChar positiveChar)
    : Range         (),
      m_negativeChar(negativeChar),
      m_positiveChar(positiveChar),
      m_value       (m_positiveChar)
{

    /* NOP */

}

/*
 * Helper function for changing the positive and negative character representation dynamically, if needed
 */
void RangeChar::setRange(QChar negativeChar, QChar positiveChar){

    m_negativeChar = negativeChar;
    m_positiveChar = positiveChar;

}

/*
 * Attempts to flip the negative state to a positive state.
 * This will return false if it's already in the positive state.
 */
bool RangeChar::increment(int){

    bool incremented(false);
    if(m_value != m_positiveChar){

        incremented = true;
        m_value = m_positiveChar;
        m_dirty = true;

    }

    return incremented;

}

/*
 * Attempts to flip the positive state to a negative state.
 * This will return false if it's already in the negative state.
 */
bool RangeChar::decrement(int){

    bool decremented(false);
    if(m_value != m_negativeChar){

        m_value = m_negativeChar;
        decremented = true;
        m_dirty = true;

    }

    return decremented;

}

/*
 * Should always return 1, since this represents a QChar
 */
int RangeChar::valueLength(){

    return 1;

}

/*
 * Should always return 1, since this represents a QChar
 */
int RangeChar::rangeLength(){

    return 1;

}

/*
 * Returns the current character state, as a string
 */
QString RangeChar::valueStr(){

    return QString(m_value);

}

/*
 * Returns "RangeChar"
 * Used to avoid dynamic casting whenever possible
 */
QString RangeChar::rangeType(){

    return "RangeChar";

}

/*
 * Unused by this Range type, returns 1LL no matter what
 */
long long RangeChar::divisor(){

    return 1LL;

}

/*
 * Allows you to explicitly set the value to be the positive or negative state.
 * Index must always be equal to this Range's start or end index.
 * Handles case insensitivity for the user implicitly.
 */
bool RangeChar::setValueForIndex(const QChar &value, int index){

    bool valueWasSet = false;

    //Lazily evaluate
    if( (index == m_charIndexEnd) && (value.toLower() != m_value.toLower()) ){

        //Check case insensitively
        if( (value.toLower() == m_positiveChar.toLower()) || (value.toLower() == m_negativeChar.toLower()) ){

            m_value     = m_value.isUpper() ? value.toUpper() : value.toLower();
            m_dirty     = true;
            valueWasSet = true;

        }

    }

    return valueWasSet;

}

/* --- RangeStringConstant definitions --- */

/*
 * Value Constructor
 */
RangeStringConstant::RangeStringConstant(const QString& stringPlaceHolder)
    : Range  (),
      m_value(stringPlaceHolder)
{

    /* NOP */

}

/*
 * Unused by this type.
 * Forwards the call to its left Range, otherwise will always return true.
 */
bool RangeStringConstant::increment(int){

    bool incremented(true);
    if(m_leftRange != nullptr){

        incremented = m_leftRange->increment(0);

    }

    return incremented;
}

/*
 * Unused by this type.
 * Forwards the call to its left Range, otherwise will always return true.
 */
bool RangeStringConstant::decrement(int){

    bool decremented(true);
    if(m_leftRange != nullptr){

        decremented = m_leftRange->decrement(0);

    }

    return decremented;

}

/*
 * Returns the length of the string this Range was constructed with
 */
int RangeStringConstant::valueLength(){

    return m_value.length();

}

/*
 * Returns the length of the string this Range was constructed with
 */
int RangeStringConstant::rangeLength(){

    return m_value.length();

}

/*
 * Returns the string this Range was constructed with
 */
QString RangeStringConstant::valueStr(){

    return m_value;

}

/*
 * Returns "RangeStringConstant"
 * Used to avoid dynamic casting whenever possible
 */
QString RangeStringConstant::rangeType(){

    return "RangeStringConstant";

}

/*
 * Unused by this Range type.
 * Always returns 1LL;
 */
long long RangeStringConstant::divisor(){

    return 1LL;

}

/*
 * Unused by this Range type.
 * Always returns true.
 * This is supposed to be a constant type, so it should be immutable.
 */
bool RangeStringConstant::setValueForIndex(const QChar&, int){

    return true;

}

/* --- RangeInt definitions --- */

/*
 * Value Constructor
 */
RangeInt::RangeInt(long long range, long long divisor, bool carryOrBorrowFromLeft)
    : Range                  (),
      m_range                (1LL),
      m_value                (0LL),
      m_divisor              (divisor),
      m_carryOrBorrowFromLeft(carryOrBorrowFromLeft)
{

    setRange(range);

}

/*
 * Convenience function to change the maximum value of the Range dynamically.
 * Ensures if lowering the range that the current stored value is still valid.
 */
bool RangeInt::setRange(long long range){

    bool successful(true);

    if(range > 0LL){

        m_range = range;
        if(m_value > m_range){

            m_value = m_range;
            m_dirty = true;

        }

    }else{

        std::cerr << QString("Error. RangeInt::setRange(long long range = %0) must be called with a range > 0.").arg(range).toStdString() << std::endl;
        successful = false;

    }

    return successful;

}

/*
 * Convenience function to change the divisor of the Range dynamically.
 * This will affect the decimal representation in the context of a PositionalLineEdit::value(...) call.
 */
bool RangeInt::setDivisor(long long divisor){

    bool successful(true);

    if(divisor > 0LL){

        m_divisor = divisor;

    }else{

        std::cerr << QString("Error. RangeInt::setDivisor(long long divisor = %0) must be called with a divisor >= 0.").arg(divisor).toStdString() << std::endl;
        successful = false;

    }

    return successful;

}

/*
 * Convenience function to help set the value, while ensuring the range limits are being observed, when either positive or negative.
 * For example, if the value is 70, but our range is 59, this will truncate the value to just set 59.
 * This will always be called before adjacent Ranges were incremented accordingly due to what this value is attempting to become.
 */
void RangeInt::setValue(long long value){

    m_value = value;
    if(m_value >= 0LL && m_value > m_range){

        m_value = m_range;

    }else if(m_value <= 0LL && m_value < -m_range){

        m_value = -m_range;

    }

}

/*
 * Helper function for dealing with over and underflow
 */
long long safeAdd(long long a, long long b){

    long long retVal = 0LL;
    if(a > 0 && b > LLONG_MAX - a){

        retVal = LLONG_MAX;

    }else if (a < 0 && b < LLONG_MIN - a){

        retVal = LLONG_MIN;

    }else{

        retVal = a + b;

    }

    return retVal;

}

/*
 * Will increment the value at the significant figure's index.
 * If this value overflows above its maximum range, it will attempt to carry the increment operation
 * over to a Range to its left, if possible.
 */
bool RangeInt::increment(int index){

    //Production::Note: The index refers to the significant figure index (right to left indexing)
    //This determines if we're at the "ones", "tens", "hundreds", etc. place to determine
    //what this widget will increment by

    long long valueToIncrementBy = std::pow(10LL, index);
    long long originalValue      = m_value;

    //Incrementing a positive number (Should make the number diverge from 0 (i.e. 20 + 10 = 30)
    if(m_value > 0LL){

        if(safeAdd(m_value, valueToIncrementBy) > m_range){

            if(m_leftRange != nullptr && m_carryOrBorrowFromLeft){

                //If we can increment our left Range
                if(m_leftRange->increment(0)){

                    setValue(m_value + (valueToIncrementBy - m_range - 1LL));

                }
                //We've hit our maximum value if we couldn't increment recursively
                else{

                    setValue(m_range);

                }

            }
            //We've hit our maximum value if we couldn't increment recursively
            else{

                setValue(m_range);

            }

        }
        //Else we don't have an addition carry, so we can just normally add and not affect our neighboring left range
        else{

            setValue(m_value + valueToIncrementBy);

        }

    }
    //Incrementing a negative number (Should make the number approach 0 (i.e. -20 + 10 = -10)
    else if(m_value < 0LL){

        //If we increment our value above 0, but we're supposed to be negative (i.e. -05 + 10 = -55 && increment the left)
        if(safeAdd(m_value, valueToIncrementBy) > 0LL){

            //If all values to our left are zeroed out, but we can flip the RangeChar (i.e. S -> N or W -> E),
            //then our addition carry will be satisfied by the RangeChar's incrementation
            bool flippingFromNegToPos = allValuesToLeftAreZero() && leftMostRange() != nullptr && leftMostRange()->increment(0);

            if(flippingFromNegToPos){

                setValue(-m_value);

            }
            //If not everything to the left is zeroed out, that means we should forward the carry bit to our left range
            else if(m_carryOrBorrowFromLeft && m_leftRange != nullptr && m_leftRange->increment(0)){

                //(i.e. S01 05' -> S00 55')
                setValue(-m_range + (valueToIncrementBy + m_value - 1LL));

            }
            //If there's no left ranges at all, attempt to treat this as a stand alone value that can be made positive
            else{

                setValue(m_value + valueToIncrementBy);

            }

        }
        //Else we can just increment because we're still going to be negative (i.e. -15 + 10 = -05)
        else{

            setValue(m_value + valueToIncrementBy);

        }

    }
    //This is a special case, depending on if our left-most RangeChar is positive or not
    else{

        //If the whole value is positive, we can safely increment
        if(leftMostRangeCharSign() == true && safeAdd(m_value, valueToIncrementBy) < m_range){

            setValue(m_value + valueToIncrementBy);

        }
        //If the whole value is negative (Or we're exceeding our range), we need to check if everything to our left is zeroed out or not
        else{

            if(allValuesToLeftAreZero() && (safeAdd(m_value, valueToIncrementBy) < m_range) ){

                //Should increment the RangeChar from its negative sign to its positive sign
                if(leftMostRange() != nullptr && leftMostRange()->increment(0)){

                    setValue(m_value + valueToIncrementBy);

                }

            }else{

                //We're actually incrementing a negative left neighboring range instead
                if(m_carryOrBorrowFromLeft && m_leftRange != nullptr && m_leftRange->increment(0)){

                    setValue(-m_range - 1LL + valueToIncrementBy);

                }else{

                    setValue(-m_range);

                }

            }

        }

    }

    m_dirty = (m_value != originalValue);

    return m_dirty;

}

/*
 * Will decrement the value at the significant figure's index.
 * If this value underflows below its negated maximum range, it will attempt to carry the decrement operation
 * over to a Range to its left, if possible.
 */
bool RangeInt::decrement(int index){

    //Production::Note: The index refers to the significant figure index (right to left indexing)
    //This determines if we're at the "ones", "tens", "hundreds", etc. place to determine
    //what this widget will decrement by

    long long valueToDecrementBy = std::pow(10LL, index);
    long long originalValue      = m_value;

    //Decrementing a positive number (i.e. 20 - 10 = 10)
    if(m_value > 0LL){

        //We're borrowing a negative from our left (if all non-zero) or the rangeChar
        if(m_carryOrBorrowFromLeft && (safeAdd(m_value, -valueToDecrementBy) < 0LL) ){

            bool flippingFromPosToNeg = allValuesToLeftAreZero() && leftMostRange() != nullptr && leftMostRange()->decrement(0);

            if(flippingFromPosToNeg){

                setValue(-m_value);

            }else if(m_leftRange != nullptr && m_leftRange->decrement(0)){

                //(i.e. N01 05' - 10' -> N00 55')
                setValue(m_range - (llabs(valueToDecrementBy - m_value) - 1LL));

            }else{

                setValue(m_value - valueToDecrementBy);

            }

        }else{

            setValue(safeAdd(m_value, -valueToDecrementBy));

        }

    }
    //Decrementing a negative number (i.e. -20 - 10 = -30)
    else if(m_value < 0LL){

        //If we underflow (i.e. -55 - 10 = -05 IF we can borrow from our left)
        if(safeAdd(m_value, -valueToDecrementBy) < -m_range){

            //If we can decrement the left range
            if(m_carryOrBorrowFromLeft && m_leftRange != nullptr && m_leftRange->decrement(0)){

                //(i.e. -55 - 10 + 59 + 1 = -65 + 60 = -5)
                setValue(m_value - valueToDecrementBy + m_range + 1LL);

            }else{

                setValue(-m_range);

            }

        }else{

            //(i.e. -45 - 10 = -55)
            setValue(safeAdd(m_value, -valueToDecrementBy));

        }

    }
    //This is a special case, depending on if our left neighbor is the RangeChar or not
    else{

        if(leftMostRangeCharSign() == true){

            if(allValuesToLeftAreZero() == true){

                //Will attempt to flip the 'N' -> 'S' and borrow from the sign to allow us to become negative
                if(leftMostRange() != nullptr){

                    leftMostRange()->decrement(0);

                }

                //If there is no RangeChar, then let us do it anyway and assume -m_range is our limit
                setValue(safeAdd(m_value, -valueToDecrementBy));

            }
            //Else not everything to our left is negative and we should be able to borrow from our left
            else{

                if(m_carryOrBorrowFromLeft && m_leftRange != nullptr && m_leftRange->decrement(0)){

                    //(i.e. N01 00' - 10' -> N00 50')
                    setValue(m_range - (llabs(valueToDecrementBy - m_value) - 1LL));

                }else{

                    setValue(safeAdd(m_value, -valueToDecrementBy));

                }

            }

        }
        //Else we're negative already, so we should be able to just subtract
        else{

            setValue(safeAdd(m_value, -valueToDecrementBy));

        }

    }

    m_dirty = (m_value != originalValue);

    return m_dirty;

}

/*
 * The length of this Range's value when converted to a string, without preleading zeroes.
 */
int RangeInt::valueLength(){

    return QString::number(llabs(m_value)).length();

}

/*
 * The length of this Range's stringified maximum value.
 */
int RangeInt::rangeLength(){

    return QString::number(m_range).length();

}

/*
 * Returns the stringified representation of this Range's value.
 * If the current number mismatches the amount of significant figures when compared to its maximum value,
 * it will prepend leading '0' characters to pad the value properly to maintain the same stringified length as its maximum range's string.
 */
QString RangeInt::valueStr(){

    return QString("0").repeated(rangeLength() - valueLength()) + QString::number(llabs(m_value));

}

/*
 * Returns "RangeInt"
 * Used to avoid dynamic casting whenever possible
 */
QString RangeInt::rangeType(){

    return "RangeInt";

}

/*
 * Returns the divisor of this Range to determine this Range's true decimal value
 * in the context of a PositionalLineEdit::value(...) call
 */
long long RangeInt::divisor(){

    return m_divisor;

}

/*
 * Attempts to replace the character of this Range's valueStr() call at index.
 * If the presumed stringified number returned can be casted to an int, it will attempt
 * to override the value held, keeping range constraints in mind.
 */
bool RangeInt::setValueForIndex(const QChar& value, int index){

    bool valueWasSet = false;

    //This allows single digit replacement while getting keyboard input from the user.
    //For example, Degrees goes from [000, 180]
    //We attempt to replace the value for index string and convert the value back to an int and then verify it's within range.
    //(i.e. value = 7 && index == 1, -> [070]
    if(QRegExp("\\d").exactMatch(value)){

        bool success(false);
        long long attemptedValue = valueStr().replace(index, 1, value).toLongLong(&success) * (m_value >= 0LL ? 1LL : -1LL);

        if(success && llabs(attemptedValue) <= m_range){

            m_value = attemptedValue;
            m_dirty = true;
            valueWasSet = true;

        }

    }

    return valueWasSet;

}
