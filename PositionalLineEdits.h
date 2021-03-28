#ifndef POSITIONALLINEEDITS_H
#define POSITIONALLINEEDITS_H

#include <QLineEdit>
#include <QPointer>

class TrianglePaintedButton;
class QPushButton;
class QMenu;

struct RangeInt;

/*
 * Think of the RangeInfo as like a node.
 * When we're in the confines of a node, they
 * have their own indexing style and validation rules.
 * For example, the degrees for latitude only allow
 * S90 to N90.
 */
struct Range{

    Range();
    virtual ~Range();

    virtual bool      increment(int index) = 0;
    virtual bool      decrement(int index) = 0;
    virtual int       valueLength()  = 0;
    virtual int       rangeLength()  = 0;
    virtual QString   valueStr()     = 0;
    virtual QString   rangeType()    = 0;
    virtual int       divisor()      = 0;
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

class PositionalLineEdits : public QLineEdit{

    Q_OBJECT

public:

    enum Type{

        LATITUDE,
        LONGITUDE,
        INVALID

    };

    PositionalLineEdits(QWidget* parent = nullptr);

    ~PositionalLineEdits();

    void setPrecision(int decimals);

    bool setValueForIndex(QString value, int index);

    double textToDecimalValue();

    void setTextFromDecimalValue(double decimalDegrees);

protected:

    void setupIncrementAndDecrementButtons();

    void createCustomContextMenu();

    Range* findAdjacentNonStringConstantRange(Range* range, bool leftRange);

    Range* getRangeForIndex(int index);

    void scrapeTextFromRangeValue(Range* range, bool overrideBeingDirty = false);

    void syncRangeEdges();

    void scrapeDirtiedRanges(bool overrideBeingDirty = false);

    void clearCurrentValidators();

    void increment();

    void decrement();

    void seekLeft();

    void seekRight();

    void maximumExceededFixup();

    double sumRangeInts();

    void syncRangeSigns();

protected slots:

    void keyPressEvent(QKeyEvent* keyEvent) override;

    void focusOutEvent(QFocusEvent* focusEvent) override;

    void paintEvent(QPaintEvent* paintEvent) override;

    void resizeEvent(QResizeEvent* resizeEvent) override;

    void wheelEvent(QWheelEvent* wheelEvent) override;

    void showContextMenu(const QPoint& pos);

    void cursorPositionChangedEvent(int prev, int cur);

    void selectionChangedEvent();

    void copyTextToClipboard();

    void copyDecimalToClipboard();

    void clearText();

public:

    Type m_type;

    QList<Range*> m_ranges;

    //This determines if m_decimals should exist
    int       m_decimals;
    RangeInt* m_decimalRange;
    double    m_undisplayedPrecision;
    int       m_maxAllowableValue;
    bool      m_showFocusFromButtonHovering;
    int       m_prevCursorPosition;

    QPointer<TrianglePaintedButton> m_incrementButton;
    QPointer<TrianglePaintedButton> m_decrementButton;

    QPointer<QMenu>       m_customContextMenu;

    RangeChar*           m_degreeChar;
    RangeInt*            m_degreeInt;
    RangeStringConstant* m_degreeSymbol;
    RangeInt*            m_minuteInt;
    RangeStringConstant* m_minuteSymbol;
    RangeInt*            m_secondsInt;
    RangeStringConstant* m_secondSymbol;

};

#endif // POSITIONALLINEEDITS_H
