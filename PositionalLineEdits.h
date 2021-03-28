#ifndef POSITIONALLINEEDITS_H
#define POSITIONALLINEEDITS_H

#include <QLineEdit>
#include <QPointer>

class Range;
class RangeChar;
class RangeInt;
class RangeStringConstant;

class TrianglePaintedButton;
class QPushButton;
class QMenu;
class QAction;

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

    void setActiveIndexHighlightColor(const QColor& highlightColor, bool implicitlyMakeSemiTransparent = true);

protected:

    void setupIncrementAndDecrementButtons();

    void createCustomContextMenu();

    Range* findAdjacentNonStringConstantRange(Range* range, bool seekLeftRange);

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

    void focusInEvent(QFocusEvent* focusEvent) override;

    void focusOutEvent(QFocusEvent* focusEvent) override;

    void paintEvent(QPaintEvent* paintEvent) override;

    void resizeEvent(QResizeEvent* resizeEvent) override;

    void wheelEvent(QWheelEvent* wheelEvent) override;

    void showContextMenu(const QPoint& pos);

    void cursorPositionChangedEvent(int prev, int cur);

    void selectionChangedEvent();

    void copyTextToClipboard();

    void copyDecimalToClipboard();

    void pasteAsDecimalFromClipboard();

    void clearText();

public:

    QList<Range*> m_ranges;

    //This determines if m_decimals should exist
    int       m_decimals;
    double    m_undisplayedPrecision;
    int       m_maxAllowableValue;    
    int       m_prevCursorPosition;

    QPointer<TrianglePaintedButton> m_incrementButton;
    QPointer<TrianglePaintedButton> m_decrementButton;

    QPointer<QMenu> m_customContextMenu;
    QAction*        m_copyAsTextToClipBoardAction;
    QAction*        m_copyAsDecimalToClipBoardAction;
    QAction*        m_pasteAsDecimalFromClipBoardAction;
    QAction*        m_clearAction;

    RangeChar*           m_degreeChar;
    RangeInt*            m_degreeInt;
    RangeStringConstant* m_degreeSymbol;
    RangeInt*            m_minuteInt;
    RangeStringConstant* m_minuteSymbol;
    RangeInt*            m_secondsInt;
    RangeStringConstant* m_secondSymbol;
    RangeInt*            m_decimalRange;

    QColor               m_highlightColor;

};

#endif // POSITIONALLINEEDITS_H
