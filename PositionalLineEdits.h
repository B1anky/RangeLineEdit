#ifndef POSITIONALLINEEDITS_H
#define POSITIONALLINEEDITS_H

#include <QLineEdit>

/*
 * Think of the RangeInfo as like a node.
 * When we're in the confines of a node, they
 * have their own indexing style and validation rules.
 * For example, the degrees for latitude only allow
 * S90 to N90.
 */
struct Range{

    Range(int charIndexStart, int charIndexEnd);
    virtual ~Range(){
        m_leftRange  = nullptr;
        m_rightRange = nullptr;
    }

    virtual bool increment() = 0;
    virtual bool decrement() = 0;

    //The index determines at what string location this range begins
    //(i.e. charIndex 0 for N/S or E/W, charIndex 1 for Degrees)
    int    m_charIndexStart;
    int    m_charIndexEnd;
    Range* m_leftRange;
    Range* m_rightRange;

    bool   m_dirty;

};

struct RangeChar : public Range{

    RangeChar(int charIndexStart, int charIndexEnd, QChar negativeChar, QChar positiveChar);
    void setRange(QChar negativeChar, QChar positiveChar);

    bool increment() override;
    bool decrement() override;

    QChar m_negativeChar;
    QChar m_positiveChar;
    QChar m_value;

};

struct RangeInt : public Range{

    RangeInt(int charIndexStart, int charIndexEnd, int range);
    void setRange(int range);

    bool increment() override;
    bool decrement() override;

    int m_range;
    int m_value;

};

class PositionalLineEdits : public QLineEdit{

    Q_OBJECT

public:

    enum Type{

        LATITUDE,
        LONGITUDE

    };

    PositionalLineEdits(Type type, int decimals, QWidget* parent = nullptr);

    void setType(Type type);

    void setDecimals(int decimals);

protected:

    Range* getRangeForIndex(int index);

    void replaceTextForRange(Range* range);

    void scrapeDirtiedRanges();

    void clearCurrentValidators();

    void setLatitudeRangeValidators();

    void setLongitudeRangeValidators();

protected slots:

    void keyPressEvent(QKeyEvent* keyEvent) override;

protected:

    Type m_type;

    QList<Range*> m_ranges;

    //This determines if m_decimals should exist
    int       m_decimals;
    RangeInt* m_decimalRange;

};

#endif // POSITIONALLINEEDITS_H
