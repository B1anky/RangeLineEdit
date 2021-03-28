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

    Range();
    virtual ~Range();

    virtual bool      increment       (int index)                       = 0;
    virtual bool      decrement       (int index)                       = 0;
    virtual int       valueLength     ()                                = 0;
    virtual int       rangeLength     ()                                = 0;
    virtual QString   valueStr        ()                                = 0;
    virtual QString   rangeType       ()                                = 0;
    virtual int       divisor         ()                                = 0;
    virtual bool      setValueForIndex(const QString& value, int index) = 0;

    virtual Range*    leftMostRange();
    virtual RangeInt* leftMostRangeInt();
    virtual bool      allValuesToLeftAreZero();
    virtual bool      leftMostRangeCharSign();

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
 */
struct RangeChar : public Range{

    RangeChar(QChar negativeChar, QChar positiveChar);
    void setRange(QChar negativeChar, QChar positiveChar);

    bool    increment       (int index = 0)                   override;
    bool    decrement       (int index = 0)                   override;
    int     valueLength     ()                                override;
    int     rangeLength     ()                                override;
    QString valueStr        ()                                override;
    QString rangeType       ()                                override;
    int     divisor         ()                                override;
    bool    setValueForIndex(const QString& value, int index) override;

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

    RangeStringConstant(QString stringPlaceHolder);

    bool    increment  (int index = 0)                        override;
    bool    decrement  (int index = 0)                        override;
    int     valueLength()                                     override;
    int     rangeLength()                                     override;
    QString valueStr   ()                                     override;
    QString rangeType  ()                                     override;
    int     divisor    ()                                     override;
    bool    setValueForIndex(const QString& value, int index) override;

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
 */
struct RangeInt : public Range{

    RangeInt(int range, int divisor);
    void setRange(int range);
    void setDivisor(int divisor);

    bool    increment       (int index)                       override;
    bool    decrement       (int index)                       override;
    int     valueLength     ()                                override;
    int     rangeLength     ()                                override;
    QString valueStr        ()                                override;
    QString rangeType       ()                                override;
    int     divisor         ()                                override;
    bool    setValueForIndex(const QString& value, int index) override;

    int m_range;
    int m_value;
    int m_divisor;

};

#endif // RANGES_H
