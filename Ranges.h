#ifndef RANGES_H
#define RANGES_H

#include <QString>

struct RangeInt;

/*! struct Range
 * Base class implementation for all Ranges.
 * Uses RTTI as much as possible to avoid casting in use of the generic class.
 */
struct Range{

    Range();
    virtual ~Range();

    virtual bool      increment(int index) = 0;
    virtual bool      decrement(int index) = 0;
    virtual int       valueLength() = 0;
    virtual int       rangeLength() = 0;
    virtual QString   valueStr()    = 0;
    virtual QString   rangeType()   = 0;
    virtual int       divisor()     = 0;
    virtual bool      setValueForIndex(const QString& value, int index) = 0;

    virtual Range*    leftMostRange();
    virtual RangeInt* leftMostRangeInt();
    virtual bool      allValuesToLeftAreZero();
    virtual bool      leftMostRangeCharSign();

    //The index determines at what string location this range begins
    //(i.e. charIndex 0 for N/S or E/W, charIndex 1 for Degrees)
    int    m_charIndexStart;
    int    m_charIndexEnd;
    Range* m_leftRange;
    Range* m_rightRange;

    bool   m_dirty;

};

struct RangeChar : public Range{

    RangeChar(QChar negativeChar, QChar positiveChar);
    void setRange(QChar negativeChar, QChar positiveChar);

    bool    increment   (int index = 0) override;
    bool    decrement   (int index = 0) override;
    int     valueLength()  override;
    int     rangeLength()  override;
    QString valueStr()     override;
    QString rangeType()    override;
    int     divisor()      override;
    bool    setValueForIndex(const QString& value, int index) override;

    QChar m_negativeChar;
    QChar m_positiveChar;
    QChar m_value;

};

/*
 * Used for constant strings that should be immutable
 */
struct RangeStringConstant : public Range{

    RangeStringConstant(QString stringPlaceHolder);

    bool    increment   (int index = 0) override;
    bool    decrement   (int index = 0) override;
    int     valueLength() override;
    int     rangeLength() override;
    QString valueStr()    override;
    QString rangeType()   override;
    int     divisor()      override;
    bool    setValueForIndex(const QString& value, int index) override;

    QString m_value;

};

struct RangeInt : public Range{

    RangeInt(int range, int divisor);
    void setRange(int range);
    void setDivisor(int divisor);

    bool    increment   (int index) override;
    bool    decrement   (int index) override;
    int     valueLength()  override;
    int     rangeLength()  override;
    QString valueStr()     override;
    QString rangeType()    override;
    int     divisor()      override;
    bool    setValueForIndex(const QString& value, int index) override;

    int m_range;
    int m_value;
    int m_divisor;

};

#endif // RANGES_H
