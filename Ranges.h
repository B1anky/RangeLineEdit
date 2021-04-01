#ifndef RANGES_H
#define RANGES_H

#include <QString>

struct RangeInt;

/*! struct Range
 *
 * Base class implementation for all Ranges.
 * Uses RTTI as much as possible to avoid casting in use of the generic class.
 * Forces subclasses to implement all required behavior via pure virtual functions.
 *
 * All Ranges have a start and an end index. This denotes their location in a fully qualified string.
 * No two ranges in the same PositionalLineEdit should have an overlap, otherwise there will be issues.
 * The PositionalLineEdit is supposed to be able to stitch a LinkedList-like structure of Range types
 * together for the user via helper functions.
 *
 */
struct Range{

    /*
     * Default Constructor
     */
    Range();

    /*
     * Destructor
     * Nulls out left and right ranges
     */
    virtual ~Range();

    /*
     * Pure virtual
     * Attempts to increment the significant figure's index by 1.
     * @PARAM int index - Significant figure's index (Right to left indexing)
     */
    virtual bool increment(int index) = 0;

    /*
     * Pure virtual
     * Attempts to decrement the significant figure's index by 1.
     * @PARAM int index - Significant figure's index (Right to left indexing)
     */
    virtual bool decrement(int index) = 0;

    /*
     * Pure virtual
     * The length of the value, when converted to its stringified version
     */
    virtual int valueLength() = 0;

    /*
     * Pure virtual
     * The length of the maximum value allowed (inclusive), when converted to its stringified version
     */
    virtual int rangeLength() = 0;

    /*
     * Pure virtual
     * The value represented as a string
     */
    virtual QString valueStr() = 0;

    /*
     * Pure virtual
     * The subclass type's string
     * Used to avoid dynamic casting whenever possible
     */
    virtual QString rangeType() = 0;

    /*
     * Pure virtual
     * The divisor that should be used to generically derive a contextualized numerical value from its base numerical value.
     * This will generally be used via value to contextualize this Range's value against its start and end index location.
     */
    virtual int divisor() = 0;

    /*
     * Pure virtual
     * This will attempt to replace the character at index.
     * Subclassed validation rules differ.
     * @PARAM const QChar& value - The new character value
     * @PARAM int          index - The index in the stringified representation to be replaced (Left to right)
     */
    virtual bool setValueForIndex(const QChar& value, int index) = 0;

    /*
     * Attempts to return whichever Range type is the head of the Ranges.
     * Will return nullptr if this is called on the head.
     */
    virtual Range* leftMostRange();

    /*
     * Attempts to return whichever RangeInt type is the most significant value of all Ranges.
     * Will return itself if it's already the left-most RangeInt.
     * Will return nullptr if this is not a RangeInt and there are no RangeInt types to its left.
     */
    virtual RangeInt* leftMostRangeInt();

    /*
     * Checks all RangeInt types to the left of `this` for a non-zero RangeInt.
     * Used in conjunction with RangeInt's increment and decrement specifically for edge case behavior.
     */
    virtual bool allValuesToLeftAreZero();

    /*
     * Returns if the left-most RangeChar is currently in its positive (true) or negative (false) state.
     * Used in conjunction with RangeInt's increment and decrement specifically for edge case behavior.
     */
    virtual bool leftMostRangeCharSign();

    int    m_charIndexStart;
    int    m_charIndexEnd;
    Range* m_leftRange;
    Range* m_rightRange;
    bool   m_dirty;

};

/*! struct RangeChar
 *
 * Derived class of Range.
 * Acts as a positive or negative sign, but allows the usage of characters.
 * Usage examples would include:
 *     1.    latitude: ('S', 'N')
 *     2.   longitude: ('W', 'E')
 *     3. normal sign: ('-', '+')
 *
 * Incrementing a RangeChar implies the negative sign is flipping to its positive character, while
 * decrementing a RangeChar implies the positive sign is flipping to its negative character.
 *
 * Production::Note: This class should only be used as the head Range when used in a PositionalLineEdit.
 * Production::Note: There should only be one of these classes used in a PositionalLineEdit.
 *
 */
struct RangeChar : public Range{

    /*
     * Value Constructor
     * @PARAM QChar negativeChar - The value that represents this Range's negative state
     * @PARAM QChar positiveChar - The value that represents this Range's positive state
     */
    RangeChar(QChar negativeChar, QChar positiveChar);

    /*
     * Helper function for changing the positive and negative character representation dynamically, if needed
     * @PARAM QChar negativeChar - The value that represents this Range's negative state
     * @PARAM QChar positiveChar - The value that represents this Range's positive state
     */
    void setRange(QChar negativeChar, QChar positiveChar);

    /*
     * Attempts to flip the negative state to a positive state.
     * This will return false if it's already in the positive state.
     * @PARAM int index - Unused for this Range type, this type can only ever have a single character representation
     */
    bool increment(int index = 0) override;

    /*
     * Attempts to flip the positive state to a negative state.
     * This will return false if it's already in the negative state.
     * @PARAM int index - Unused for this Range type, this type can only ever have a single character representation
     */
    bool decrement(int index = 0) override;

    /*
     * Should always return 1, since this represents a QChar
     */
    int valueLength() override;

    /*
     * Should always return 1, since this represents a QChar
     */
    int rangeLength() override;

    /*
     * Returns the current character state, as a string
     */
    QString valueStr() override;

    /*
     * Returns "RangeChar"
     * Used to avoid dynamic casting whenever possible
     */
    QString rangeType() override;

    /*
     * Unused by this Range type, returns 1 no matter what
     */
    int divisor() override;

    /*
     * Allows you to explicitly set the value to be the positive or negative state.
     * Index must always be equal to this Range's start or end index.
     * Handles case insensitivity for the user implicitly.
     * @PARAM const QChar& value - The value to attempt a state change with
     * @PARAM int          index - The index requested to make the state change at
     */
    bool setValueForIndex(const QChar& value, int index) override;

    QChar m_negativeChar;
    QChar m_positiveChar;
    QChar m_value;

};

/*! struct RangeStringConstant
 *
 * Derived class of Range.
 * Acts as a constant, immutable character.
 * Usage examples would include:
 *     1. degrees: (" ° ")
 *     2. decimal: (" . ")
 *     3.  minute: (" ' ")
 *     4. seconds: (" '' ")
 *
 * Incrementing / Decrementing a RangeStringConstant implies the operation
 * is passed along to its left Range, if it has one, otherwise it will always be true.
 *
 */
struct RangeStringConstant : public Range{

    /*
     * Value Constructor
     * @PARAM const QString& stringPlaceHolder - The value that this String constant will display as
     */
    RangeStringConstant(const QString& stringPlaceHolder);

    /*
     * Unused by this type.
     * Forwards the call to its left Range, otherwise will always return true.
     * @PARAM int index - Unused
     */
    bool increment(int index = 0) override;

    /*
     * Unused by this type.
     * Forwards the call to its left Range, otherwise will always return true.
     * @PARAM int index - Unused
     */
    bool decrement(int index = 0) override;

    /*
     * Returns the length of the string this Range was constructed with
     */
    int valueLength() override;

    /*
     * Returns the length of the string this Range was constructed with
     */
    int rangeLength() override;

    /*
     * Returns the string this Range was constructed with
     */
    QString valueStr() override;

    /*
     * Returns "RangeStringConstant"
     * Used to avoid dynamic casting whenever possible
     */
    QString rangeType() override;

    /*
     * Unused by this Range type.
     * Always returns 1;
     */
    int divisor() override;

    /*
     * Unused by this Range type.
     * Always returns true.
     * This is supposed to be a constant type, so it should be immutable.
     * @PARAM const QChar& value - Unused
     * @PARAM int          index - Unused
     */
    bool setValueForIndex(const QChar& value, int index) override;

    QString m_value;

};

/*! struct RangeInt
 *
 * Derived class of Range.
 * Acts as a mutable integer representation as a string.
 * Internally handles increment and decrement operators using predefined
 * ranges and handles outputting its decimal representation via is value / divisor.
 *
 * Usage examples would include:
 *     1. degrees: (180,   1)
 *     2. minutes: (59,   60)
 *     3. seconds: (59, 3600)
 *
 * Incrementing / Decrementing a RangeInt implies the operation
 * will modify the underly integer and handle carrying / borrowing
 * of the arithmetic operation to any associated Ranges.
 *
 * Production::Note: The range of the left-most RangeInt will be deemed as
 * the maximum value when used in the context of a PositionalLineEdit. (i.e. 180 for longitude, 90 for latitude)
 *
 */
struct RangeInt : public Range{

    /*
     * Value Constructor
     * @PARAM int  range                 - The maximum value of the Range (In both the positive and negative direction)
     * @PARAM int  divisor               - The divisor of the Range that will divide the stored state's value to be used to construct the
     *                                     decimal representation of this Range in the context of a PositionalLineEdit::value(...) call
     * @PARAM bool carryOrBorrowFromLeft - This will determine if this RangeInt will attempt to increment its neighbor, versus just remianing at its maximum value
     */
    RangeInt(int range, int divisor, bool carryOrBorrowFromLeft = true);

    /*
     * Convenience function to change the maximum value of the Range dynamically.
     * Ensures if lowering the range that the current stored value is still valid.
     * @PARAM int range - The maximum value of the Range (In both the positive and negative direction)
     */
    bool setRange(int range);

    /*
     * Convenience function to change the divisor of the Range dynamically.
     * This will affect the decimal representation in the context of a PositionalLineEdit::value(...) call.
     * @PARAM int divisor - The divisor of the Range that will divide the stored state's value to be used to construct the
     *                      decimal representation of this Range in the context of a PositionalLineEdit::value(...) call
     */
    bool setDivisor(int divisor);

    /*
     * Convenience function to help set the value, while ensuring the range limits are being observed, when either positive or negative.
     * For example, if the value is 70, but our range is 59, this will truncate the value to just set 59.
     * This will always be called before adjacent Ranges were incremented accordingly due to what this value is attempting to become.
     * @PARAM int value - The value to attempt setting our current value to. Will be limited by this RangeInt's range.
     */
    void setValue(int value);

    /*
     * Will increment the value at the significant figure's index.
     * If this value overflows above its maximum range, it will attempt to carry the increment operation
     * over to a Range to its left, if possible.
     * @PARAM int index - The significant figure's index (Right to left indexing) to perform the increment operation on
     */
    bool increment(int index) override;

    /*
     * Will decrement the value at the significant figure's index.
     * If this value underflows below its negated maximum range, it will attempt to carry the decrement operation
     * over to a Range to its left, if possible.
     * @PARAM int index - The significant figure's index (Right to left indexing) to perform the decrement operation on
     */
    bool decrement(int index) override;

    /*
     * The length of this Range's value when converted to a string, without preleading zeroes.
     */
    int valueLength() override;

    /*
     * The length of this Range's stringified maximum value.
     */
    int rangeLength() override;

    /*
     * Returns the stringified representation of this Range's value.
     * If the current number mismatches the amount of significant figures when compared to its maximum value,
     * it will prepend leading '0' characters to pad the value properly to maintain the same stringified length as its maximum range's string.
     */
    QString valueStr () override;

    /*
     * Returns "RangeInt"
     * Used to avoid dynamic casting whenever possible
     */
    QString rangeType() override;

    /*
     * Returns the divisor of this Range to determine this Range's true decimal value
     * in the context of a PositionalLineEdit::value(...) call
     */
    int divisor() override;

    /*
     * Attempts to replace the character of this Range's valueStr() call at index.
     * If the presumed stringified number returned can be casted to an int, it will attempt
     * to override the value held, keeping range constraints in mind.
     * @PARAM const QChar& value - The string, representing a numerical character, to replace a value with
     * @PARAM int          index - The index in valueStr() to replace the adjacent value argument at
     */
    bool setValueForIndex(const QChar& value, int index) override;

    int  m_range;
    int  m_value;
    int  m_divisor;
    bool m_carryOrBorrowFromLeft;

};

#endif // RANGES_H
