#ifndef RANGELINEEDIT_H
#define RANGELINEEDIT_H

#include <QLineEdit>
#include <QPointer>

#include "RangeLineEdit.h"
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

/*! class RangeLineEdit
 *
 * Derived type of QLineEdit.
 * Employs the Range type class suite to manage a pseudo RegExpr
 * for Character and Integer states. It will tether multiple Ranges
 * together for the user in the order they're pushed back to the internal
 * list of Ranges. Has the following functionality:
 *     1. Always guaranteed valid state
 *     2. Overridden context menu options for:
 *         a. Copying the text to your clipboard as a string
 *         b. Copying the text as a decimal to your clipboard as a string
 *         c. Pasting a valid decimal value
 *         d. Clearing the value and zeroing all RangeInts
 *     3. Supports KeyPressEvent for:
 *         a. Alphanumerics
 *         b. Up and Down Arrows (Increment and Decrement operations, respectively)
 *         c. Left and Right Arrows (Seek left and Seek right, respectively)
 *         d. Backspace and Delete keys
 *             i.  (Backspace will zero out the current RangeInt index and move cursor left)
 *             ii. (Delete will zero out the current RangeInt index and move cursor right)
 *         e. Home and End
 *         f. Ctrl+C will copy text as decimal to clipboard
 */
template <class ValueType>
class RangeLineEdit : public QLineEdit{

public:

    /*
     * Value Constructor
     */
    RangeLineEdit(QWidget* parent)
        : QLineEdit                        (parent),
          m_ranges                         ({}),
          m_decimals                       (-1),
          m_maxAllowableValue              (0LL),
          m_prevCursorPosition             (0),
          m_incrementButton                (nullptr),
          m_decrementButton                (nullptr),
          m_customContextMenu              (nullptr),
          m_copyAsTextToClipBoardAction    (nullptr),
          m_copyAsValueToClipBoardAction   (nullptr),
          m_pasteAsValueFromClipBoardAction(nullptr),
          m_clearAction                    (nullptr),
          m_decimalRange                   (nullptr),
          m_highlightColor                 (QColor(128, 128, 128, 75))
    {

        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);
        setupIncrementAndDecrementButtons();
        createCustomContextMenu();

        connect(this, &RangeLineEdit::cursorPositionChanged,      this, &RangeLineEdit::cursorPositionChangedEvent, Qt::DirectConnection);
        connect(this, &RangeLineEdit::selectionChanged,           this, &RangeLineEdit::selectionChangedEvent,      Qt::DirectConnection);
        connect(this, &RangeLineEdit::customContextMenuRequested, this, &RangeLineEdit::showContextMenu,            Qt::DirectConnection);
        connect(this, &RangeLineEdit::textChanged,                this, &RangeLineEdit::valueChangedPrivate,        Qt::DirectConnection);

    }

    /*
     * Destructor.
     * Clears all Range validators.
     */
    ~RangeLineEdit(){

        clearCurrentValidators();

    }

    /*
     * Convenience function for dynamically changing the precision of the decimals.
     * @PARAM int decimals - Decimal precision to be displayed. Can be set to 0 which will remove precision values, if already set.
     */
    virtual void setPrecision(int decimals){

        if(m_decimals != decimals && decimals > 0){

            m_decimals = decimals;
            int currentCursorPos = this->cursorPosition();

            long long prevDivisor = 1LL;
            for(int i = m_ranges.size() - 1; i >= 0; --i){

                if(m_ranges.at(i)->rangeType() == "RangeInt" && m_ranges.at(i) != m_decimalRange){

                    prevDivisor = m_ranges.at(i)->divisor();
                    break;

                }

            }

            //Generally this occurs if we're setting our type for the first time or changing our type dynamically
            if(m_decimalRange == nullptr){

                m_decimalString = new RangeStringConstant(".");
                m_decimalRange  = new RangeInt(std::pow(10LL, m_decimals) - 1LL, std::pow(10LL, m_decimals) * prevDivisor);

                //Production::Note: If the final Range type in the current m_ranges list when initialized is a RangeStringConstant (i.e. a " '' "),
                //then they are probably attempting to make the decimal apply to its closest RangeInt, so we want to pop the previous tail,
                //append our new string constant for the decimal point and RangeInt for the decimals, then append the previous tail back on
                ::Range* secondSymbol = nullptr;

                //For example, we may have to pop something like " '' " from a seconds symbol off the back and move it to the right of the decimal ranges
                if(m_ranges.empty() == false && m_ranges.last()->rangeType() == "RangeStringConstant"){

                    secondSymbol = m_ranges.last();
                    m_ranges.pop_back();

                }

                m_ranges << m_decimalString << m_decimalRange;

                //This is only optionally done if the above assumptions were true, otherwise we're directly appending our two new Range types for decimal representation
                if(secondSymbol != nullptr){

                    m_ranges << secondSymbol;

                }

            }

            m_decimalRange->setRange(std::pow(10LL, m_decimals) - 1LL);
            m_decimalRange->setDivisor(std::pow(10LL, m_decimals) * prevDivisor);

            clear();
            syncRangeEdges();

            setCursorPosition(currentCursorPos);

        }
        //Handle removing and cleaning up the decimal range if it previously existed
        else if(decimals == 0 && m_decimalRange != nullptr){

            Range* currentTail = nullptr;
            m_decimals = decimals;

            //Pop the Seconds String Constant, the decimal Range, and the decimal String Constant
            if(m_ranges.isEmpty() == false && m_ranges.last()->rangeType() == "RangeStringConstant"){

                currentTail = m_ranges.last();
                m_ranges.pop_back();

            }

            m_ranges.pop_back();
            delete m_decimalString;

            m_ranges.pop_back();
            delete m_decimalRange;

            m_decimalString = nullptr;
            m_decimalRange  = nullptr;

            if(currentTail != nullptr){

                m_ranges << currentTail;

            }

            clear();
            syncRangeEdges();

        }

    }

    /*
     * Delegates a call to the proper Range for the given index to call its own Range::setValueForIndex(...)
     * @PARAM const QChar& value - The String to set at the given index
     * @PARAM int          index - The index used to lookup the held Range
     */
    virtual bool setValueForIndex(const QChar& value, int index){

        bool valueWasSet = false;
        ::Range* range = getRangeForIndex(index);
        if(range != nullptr){

            valueWasSet = range->setValueForIndex(value, index - range->m_charIndexStart);
            if(valueWasSet){

                syncRangeSigns();
                maximumExceededFixup();
                scrapeDirtiedRanges();
                setCursorPosition(index);

            }

        }

        return valueWasSet;

    }

    /*
     * Convenience function for setting the current active index's color
     * @PARAM const QColor& highlightColor                - The color to set
     * @PARAM bool          implicitlyMakeSemiTransparent - Whether or not to implicitly "tint" the provided color by setting its alpha to 75/255
     */
    void setActiveIndexHighlightColor(const QColor& highlightColor, bool implicitlyMakeSemiTransparent = true){

        if(highlightColor.isValid()){

            m_highlightColor = highlightColor;

            //This will make it so a user could, for example use Qt::red, Qt::blue, Qt::green, etc.,
            //and it'll automatically make it semi-transparent without multiple lines of code on their part
            if(implicitlyMakeSemiTransparent){

                m_highlightColor.setAlpha(75);

            }

        }

    }

    /*
     * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
     * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
     * @PARAM ValueType value - The value that should be handled to populate the widget's Ranges from its specified derived type
     */
    virtual void setValue(ValueType value) = 0;

    /*
     * Force the specialized parameratized subclass to have to define how its underling Ranges should be converted to some usable value
     * This mimics Qt'isms where their widgets that have a value, normally have a callable T::value().
     */
    virtual ValueType value() = 0;

protected:

    /*
     * Helper function used by the constructor to initialize the state and connections of the increment and decrement buttons.
     */
    void setupIncrementAndDecrementButtons(){

        //Production::Note: It doesn't looks like the inverse character, but it appears properly
        m_incrementButton = new TrianglePaintedButton(TrianglePaintedButton::Direction::UP,   this);
        m_decrementButton = new TrianglePaintedButton(TrianglePaintedButton::Direction::DOWN, this);

        m_incrementButton->setMinimumWidth(15);
        m_decrementButton->setMinimumWidth(15);

        m_incrementButton->setMaximumWidth(25);
        m_decrementButton->setMaximumWidth(25);

        m_incrementButton->setMouseTracking(true);
        m_decrementButton->setMouseTracking(true);

        connect(m_incrementButton, &QPushButton::clicked, this, &RangeLineEdit<ValueType>::increment, Qt::DirectConnection);
        connect(m_decrementButton, &QPushButton::clicked, this, &RangeLineEdit<ValueType>::decrement, Qt::DirectConnection);

    }

    /*
     * Helper function used by the constructor to initialize the state and connections of the context menu.
     */
    void createCustomContextMenu(){

        setContextMenuPolicy(Qt::CustomContextMenu);
        m_customContextMenu = new QMenu(this);

        m_copyAsTextToClipBoardAction     = m_customContextMenu->addAction("Copy  [As text]");
        m_copyAsValueToClipBoardAction    = m_customContextMenu->addAction("Copy  [As value]");
        m_pasteAsValueFromClipBoardAction = m_customContextMenu->addAction("Paste [From value]");


        m_clearAction                 = m_customContextMenu->addAction("Clear");

        connect(m_copyAsTextToClipBoardAction,     &QAction::triggered, this, &RangeLineEdit<ValueType>::copyTextToClipboard,     Qt::DirectConnection);
        connect(m_copyAsValueToClipBoardAction,    &QAction::triggered, this, &RangeLineEdit<ValueType>::copyValueToClipboard,    Qt::DirectConnection);
        connect(m_pasteAsValueFromClipBoardAction, &QAction::triggered, this, &RangeLineEdit<ValueType>::pasteValueFromClipboard, Qt::DirectConnection);
        connect(m_clearAction,                     &QAction::triggered, this, &RangeLineEdit<ValueType>::clearText,               Qt::DirectConnection);

    }

    /*
     * Helper function to find the immediate left or right Range that represents an editable index among the held Ranges
     * @PARAM Range* range         - The starting range to use as a reference
     * @PARAM bool   seekLeftRange - Whether or not to seek left (true) or right (false)
     */
    ::Range* findAdjacentNonStringConstantRange(::Range* range, bool seekLeftRange){

        ::Range* adjacentRange(nullptr);
        int iterInd(0);

        if(seekLeftRange){

            iterInd = m_ranges.indexOf(range) - 1;

            while(iterInd > 0){

                if(m_ranges.at(iterInd)->rangeType() != "RangeStringConstant"){

                    adjacentRange = m_ranges.at(iterInd);
                    break;

                }

                --iterInd;

            }

        }else{

            iterInd = m_ranges.indexOf(range) + 1;

            while(iterInd < m_ranges.size()){

                if(m_ranges.at(iterInd)->rangeType() != "RangeStringConstant"){

                    adjacentRange = m_ranges.at(iterInd);
                    break;

                }

                ++iterInd;

            }

        }

        return adjacentRange;

    }

    /*
     * Helper function for returning the given Range for the index in this widget's QLineEdit::text()
     * @PARAM int index - The index to match against all Range's start and end indices
     */
    ::Range* getRangeForIndex(int index){


        ::Range* rangeForIndex(nullptr);
        foreach(::Range* range, m_ranges){

            if(index >= range->m_charIndexStart && index <= range->m_charIndexEnd){

                rangeForIndex = range;
                break;

            }

        }

        //This can occur if the cursor position is all of the way at the end
        if(rangeForIndex == nullptr && (m_ranges.isEmpty() == false) ){

            rangeForIndex = m_ranges.last();

        }

        return rangeForIndex;

    }

    /*
     * Helper function for getting the proper string representation out of a given Range
     * @PARAM Range* range              - The range to scrape the text from, if dirty
     * @PARAM bool   overrideBeingDirty - Scrape the Range's text value, regardless of it being dirty or not
     */
    void scrapeTextFromRangeValue(::Range* range, bool overrideBeingDirty = false){

        if(range->m_dirty || overrideBeingDirty){

            QString curText    = this->text();
            QString paddedText = range->valueStr();
            curText = curText.replace(range->m_charIndexStart, paddedText.length(), paddedText);

            range->m_dirty = false;

            //Production::Note: All calls to RangeLineEdits::scrapeTextFromRangeValue(...) are wrapped in
            //blockSignals(true) to prevent multiple emissions of textChanged per batch update.
            //At the end of the block we check to see if our original text no longer matches the "new"
            //scraped text and manually emit QLineEdit::textChanged(this->text()) once after unblocking signals
            setText(curText);

        }

    }

    /*
     * Helper function for tying the Ranges together properly during initialization and any subsequent calls to setPrecision(...).
     * This will sync all Range's left and right neighbors properly and initialize their start and end indices as well.
     */
    void syncRangeEdges(){

        int curRangeOffset(0);

        for(int i = 0; i < m_ranges.size() - 1; ++i){

            ::Range* leftRange  = m_ranges[i];
            ::Range* rightRange = m_ranges[i + 1];

            leftRange->m_rightRange = rightRange;
            rightRange->m_leftRange = leftRange;

            leftRange->m_charIndexStart = curRangeOffset;
            curRangeOffset += leftRange->rangeLength();
            leftRange->m_charIndexEnd = curRangeOffset - 1;

            leftRange->m_dirty  = true;
            rightRange->m_dirty = true;

        }

        //Now clean up the last indice's ranges and left range value
        if(m_ranges.size() >= 2){

            m_ranges.last()->m_leftRange = m_ranges.at(m_ranges.size() - 2);
            m_ranges.last()->m_charIndexStart = curRangeOffset;

            curRangeOffset += m_ranges.last()->rangeLength();

            m_ranges.last()->m_charIndexEnd = curRangeOffset - 1;
            m_ranges.last()->m_dirty = true;

        }

        scrapeDirtiedRanges(true);

    }

    /*
     * Helper function that delegates a call to scrapeTextFromRangeValue(...) on each held Range
     * @PARAM bool overrideBeingDirty - Scrape all of the Ranges' text values, regardless of them being dirty or not
     */
    void scrapeDirtiedRanges(bool overrideBeingDirty = false){

        QString originalText = text();

        blockSignals(true);

        foreach(::Range* range, m_ranges){

            scrapeTextFromRangeValue(range, overrideBeingDirty);

        }

        blockSignals(false);

        //Doing this will ensure one emission of QLineEdit::textChanged(...) will occur for a given batch update
        if(originalText != text()){

            emit textChanged(text());

        }

    }

    /*
     * Clears all Ranges properly, nulls out the memory, and clears the held list
     */
    virtual void clearCurrentValidators(){

        foreach(::Range* range, m_ranges){

            if(dynamic_cast<RangeInt*>(range)){

                delete dynamic_cast<RangeInt*>(range);

            }else if(dynamic_cast<RangeChar*>(range)){

                delete dynamic_cast<RangeChar*>(range);

            }else if(dynamic_cast<RangeStringConstant*>(range)){

                delete dynamic_cast<RangeStringConstant*>(range);

            }else{

                delete range;

            }

            range = nullptr;

        }

        m_ranges.clear();

    }

    /*
     * Attempts to increment the Range at the current cursor index
     */
    virtual void increment(){

        //Check if this position belongs to a valid Range
        ::Range* range = getRangeForIndex(m_prevCursorPosition);

        if(range != nullptr){

            int localRangeIndex = range->m_charIndexEnd - m_prevCursorPosition;

            if(range->rangeType() != "RangeStringConstant"){

                if(range->increment(localRangeIndex)){

                    syncRangeSigns();
                    maximumExceededFixup();
                    scrapeDirtiedRanges();

                    setCursorPosition(m_prevCursorPosition);

                }

            }

        }

    }

    /*
     * Attempts to decrement the Range at the current cursor index
     */
    virtual void decrement(){

        //Check if this position belongs to a valid Range
        ::Range* range = getRangeForIndex(m_prevCursorPosition);

        if(range != nullptr){

            int localRangeIndex = range->m_charIndexEnd - m_prevCursorPosition;

            if(range->rangeType() != "RangeStringConstant"){

                if(range->decrement(localRangeIndex)){

                    syncRangeSigns();
                    maximumExceededFixup();
                    scrapeDirtiedRanges();

                    setCursorPosition(m_prevCursorPosition);

                }

            }

        }

    }

    /*
     * Attempts to move the cursor position to the left to the next valid, editable, Range subtype
     */
    void seekLeft(){

        int focusIndex = this->cursorPosition();

        //No point to move left if we're at 0
        if(focusIndex > 0){

            //Assume this is valid and prove otherwise below
            int newCursorPosition = focusIndex - 1;

            //Check which Range belongs to the current cursor position
            ::Range* range = getRangeForIndex(focusIndex);

            if(range != nullptr){

                if(newCursorPosition < range->m_charIndexStart || range->rangeType() == "RangeStringConstant"){

                    ::Range* leftMostAdjacentRangeValue = findAdjacentNonStringConstantRange(range, true);

                    //Grab the range to its left's end index to hop properly
                    if(leftMostAdjacentRangeValue != nullptr){

                        newCursorPosition = leftMostAdjacentRangeValue->m_charIndexEnd;

                    }

                }

            }

            setCursorPosition(newCursorPosition);

        }

    }

    /*
     * Attempts to move the cursor position to the right to the next valid, editable, Range subtype
     */
    void seekRight(){

        int focusIndex = this->cursorPosition();

        //No point to move right if we're at the end
        if(focusIndex < this->text().length()){

            //Assume this is valid and prove otherwise below
            int newCursorPosition = focusIndex + 1;

            //Check which Range belongs to the current cursor position
            ::Range* range = getRangeForIndex(focusIndex);

            if(range != nullptr){

                if(newCursorPosition > range->m_charIndexEnd || range->rangeType() == "RangeStringConstant"){

                    ::Range* rightMostAdjacentRangeValue = findAdjacentNonStringConstantRange(range, false);

                    //Grab the range to its right's start index to hop properly
                    if(rightMostAdjacentRangeValue != nullptr){

                        newCursorPosition = rightMostAdjacentRangeValue->m_charIndexStart;

                    }

                }

                setCursorPosition(newCursorPosition);

            }

        }

    }

    /*
     * Helper function that ensures any changes to the value of a Range will not exceed the maximum allowable set value.
     * If the maximum is exceeded, the first-most RangeInt will be set to its range and all subsequent RangeInts will be zeroed out.
     */
    virtual void maximumExceededFixup(){

        //Assume false and prove otherwise
        bool atOrExceedsValue(false);

        //This will essentially zero out all RangeInts and set the first one to the max allowed value
        if(std::fabs(value()) >= m_maxAllowableValue){

            atOrExceedsValue = true;
            foreach(::Range* range, m_ranges){

                if(range->rangeType() == "RangeInt"){

                    static_cast<RangeInt*>(range)->m_value = 0LL;
                    range->m_dirty = true;

                }

            }

        }

        if(Q_UNLIKELY(atOrExceedsValue)){

            //Look for the first RangeInt and set it to the maximum range
            foreach(::Range* range, m_ranges){

                if(range->rangeType() == "RangeInt"){

                    static_cast<RangeInt*>(range)->m_value = m_maxAllowableValue;
                    break;

                }

            }

            //Production::Note: Any calling of QLineEdit::setText(...)
            //will set the cursorPosition to the end of the LineEdit,
            //so often you will see this getting saved off before we possibly
            //make any call to QLineEdit::setText(...) to ensure we keep our position proper
            int focusIndex = this->cursorPosition();

            syncRangeSigns();
            scrapeDirtiedRanges(true);

            setCursorPosition(focusIndex);

        }

    }

    /*
     * Ensures if the signage (+/-) changes as a result of the RangeChar being modified,
     * that all subsequent RangeInt types match the same sign (+/-) for their underlying value.
     */
    void syncRangeSigns(){

        //Assume it to be positive
        bool charSign(true);

        //Grab the first RangeChar's sign
        if( (m_ranges.isEmpty() == false) && (m_ranges.first()->rangeType() == "RangeChar") ){

            RangeChar* rangeChar = static_cast<RangeChar*>(m_ranges.first());

            charSign = (rangeChar->valueStr() == rangeChar->m_positiveChar);

        }

        //Now loop through all RangeInts to make them the same sign (All positive or all negative)
        foreach(::Range* range, m_ranges){

            if(range->rangeType() == "RangeInt"){

                RangeInt* rangeInt = static_cast<RangeInt*>(range);

                bool rangeSign = rangeInt->m_value > 0;

                if(rangeSign != charSign){

                    rangeInt->m_value *= -1;

                }

            }

        }

        int focusIndex = this->cursorPosition();

        scrapeDirtiedRanges(true);

        setCursorPosition(focusIndex);

    }

    /*
     * Returns a sum of all RangeInts' values / RangeInts' divisors, without the undisplayed precision
     */
    long double sumRangeInts(){

        long double sum(0.0L);

        foreach(::Range* range, m_ranges){

            if(range->rangeType() == "RangeInt" && range->divisor() > 1){

                sum += static_cast<long double>(static_cast<RangeInt*>(range)->m_value) / static_cast<long double>(range->divisor());

            }
            //Attempt to do the least amount of floating point arithmetic as possible to reduce precision loss
            else if(range->rangeType() == "RangeInt" && range->divisor() == 1){

                sum += static_cast<long double>(static_cast<RangeInt*>(range)->m_value);

            }

        }

        return sum;

    }

protected slots:

    /*
     * Overridden QKeyEvent
     * Manages calls to all of the helper functions to change the state of this widget
     * @PARAM QKeyEvent* keyEvent - Standard Qt QKeyEvent
     */
    void keyPressEvent(QKeyEvent* keyEvent) override{

        int key(keyEvent->key());

        //Production::Note: The order of these if statements have specific precedence.
        //The last check: `keyEvent->text().length() == 1` will trigger on any single key length,
        //so if adding new functionality, ensure that it remains as the last if check to ensure new
        //functionality isn't being skipped over.

        if(key == Qt::Key_Up){

            increment();

        }else if(key == Qt::Key_Down){

            decrement();

        }else if(key == Qt::Key_Left){

            seekLeft();

        }else if(key == Qt::Key_Right){

            seekRight();

        }else if(key == Qt::Key_Backspace){

            setValueForIndex('0', this->cursorPosition());

            seekLeft();

        }else if(key == Qt::Key_Delete){

            setValueForIndex('0', this->cursorPosition());

            seekRight();

        }else if(keyEvent->matches(QKeySequence::Copy)){

            copyValueToClipboard();

        }else if(keyEvent->matches(QKeySequence::Paste)){

            pasteValueFromClipboard();

        }else if(key == Qt::Key_Home || key == Qt::Key_End){

            QLineEdit::keyPressEvent(keyEvent);

        }
        //Production::Note: Add new `else if`s above this one
        else if(keyEvent->text().length() == 1){

            if(setValueForIndex(keyEvent->text().at(0), this->cursorPosition())){

                seekRight();

            }

        }//Production::Note: Don't even think about adding another `else if` below here

    }

    /*
     * Overridden QFocusEvent
     * Clears the widget on focus-in and repopulates it.
     * Done to ensure if an outside widget or progrmmatic functionality
     * erroneously called setText(...) on this widget, that it will reconstruct the displayed text properly
     * @PARAM QFocusEvent* focusEvent - Standard Qt QFocusEvent
     */
    void focusInEvent(QFocusEvent* focusEvent) override{

        int focusIndex = cursorPosition();

        //This serves to make sure if the user somehow improperly called setText from another context and didn't use the supplied
        //RangeLineEdits::setValue(...), that we will clear the text and reconstruct the string on focusIn.
        blockSignals(true);
        clear();
        blockSignals(false);
        scrapeDirtiedRanges(true);

        setCursorPosition(focusIndex);

        QLineEdit::focusInEvent(focusEvent);

    }

    /*
     * Overridden QFocusEvent
     * Clears the widget on focus-out and repopulates it.
     * Done to ensure if an outside widget or progrmmatic functionality
     * erroneously called setText(...) on this widget, that it will reconstruct the displayed text properly
     * @PARAM QFocusEvent* focusEvent - Standard Qt QFocusEvent
     */
    void focusOutEvent(QFocusEvent* focusEvent) override{

        int focusIndex = cursorPosition();

        //This serves to make sure if the user somehow improperly called setText from another context and didn't use the supplied
        //RangeLineEdits::setValue(...), that we will clear the text and reconstruct the string on focusOut.
        blockSignals(true);
        clear();
        blockSignals(false);
        scrapeDirtiedRanges(true);

        setCursorPosition(focusIndex);

        QLineEdit::focusOutEvent(focusEvent);

    }

    /*
     * Overridden QPaintEvent
     * Calls base class behavior before attemtping to paint a small colored rectangle around
     * the current cursor's position to denote which index is currently in an editable context
     * @PARAM QPaintEvent* paintEvent - Standard Qt QPaintEvent
     */
    void paintEvent(QPaintEvent* paintEvent) override{

        //Draw the QLineEdit as normal
        QLineEdit::paintEvent(paintEvent);

        //Below highlights the current text that has focus in the widget and will be affected by an increment, decrement, or key press operation
        if( (this->hasFocus() || m_incrementButton->underMouse() || m_decrementButton->underMouse()) && cursorPosition() <= text().length() - 1){

            QPainter painter(this);
            painter.setPen(QPen(QColor(255, 255, 255, 0)));
            painter.setBrush(QBrush(m_highlightColor));

            QFontMetrics fontMetrics(font());

            int pixelsWide = fontMetrics.horizontalAdvance(text().at(cursorPosition()));
            int pixelsHigh = fontMetrics.height();

            QPoint topLeft  = cursorRect().topLeft();

            //The topLeft start x position is too far from the blinking caret (text cursor),
            //so this offsets the x position properly to align with the blinking caret properly
            topLeft.setX(topLeft.x() + cursorRect().width() / 2);

            QPoint botRight = QPoint(topLeft.x() + pixelsWide - 1, topLeft.y() + pixelsHigh);

            painter.drawRect(QRect(topLeft, botRight));

        }

    }

    /*
     * Overridden QResizeEvent
     * Modifies the size and position of the increment and decrement push buttons
     * to always be with respect to this widget's width.
     * @PARAM QResizeEvent* resizeEvent - Standard Qt QResizeEvent
     */
    void resizeEvent(QResizeEvent* resizeEvent) override{

        //This will make it so the buttons have some wiggle room (simulates as if they're in a layout, without actually being in one)
        QSize buttonSize(width() * 0.10, height() / 2 - 1);

        //These buttons have an enforced minimum and maximum size, so it will scale slightly with the size of the widget, to an extent
        m_incrementButton->resize(buttonSize);
        m_decrementButton->resize(buttonSize);

        m_incrementButton->move(width() - m_incrementButton->width(), 1);
        m_decrementButton->move(width() - m_decrementButton->width(), height() / 2);

        QLineEdit::resizeEvent(resizeEvent);

    }

    /*
     * Overridden QWheelEvent
     * Attempts to increment on a wheel up or decrement on a wheel down event for the given active index.
     * @PARAM QWheelEvent* wheelEvent - Standard Qt QWheelEvent
     */
    void wheelEvent(QWheelEvent* wheelEvent) override{

        if(this->hasFocus()){

            if(wheelEvent->angleDelta().y() > 0){

                increment();

            }else{

                decrement();

            }

        }

        wheelEvent->accept();

    }

    /*
     * Connected to RangeLineEdit::customContextMenuRequested.
     * Invoked on a right click event and spawns a custom context menu.
     * @PARAM const QPoint& pos - The position in widget coordinates that gets mapped to global coordinates to display the context menu at
     */
    virtual void showContextMenu(const QPoint& pos){

        m_customContextMenu->exec(mapToGlobal(pos));

    }

    /*
     * Connected to RangeLineEdit::cursorPositionChanged.
     * Invoked whenever the cursor position was changed.
     * Ensures the cursor is only ever on top of an editable Range.
     * @PARAM int prev - Unused (But used to match QLineEdit's signal arguments)
     * @PARAM int cur  - The new position requested by either the user or done programmatically via setCursorPosition()
     */
    void cursorPositionChangedEvent(int, int cur){

        ::Range* range = getRangeForIndex(cur);
        if(range != nullptr && range->rangeType() == "RangeStringConstant"){

            //Go to the left, if possible, otherwise fall back to the right
            if(range->m_leftRange != nullptr){

                setCursorPosition(range->m_leftRange->m_charIndexEnd);

            }else if(range->m_rightRange != nullptr){

                setCursorPosition(range->m_rightRange->m_charIndexStart);

            }

        }
        //In the case we don't have a RangeStringConstant at the end
        else if(cur == text().length()){

            setCursorPosition(cur - 1);

        }

        if(this->hasFocus()){

            m_prevCursorPosition = cursorPosition();

        }

    }

    /*
     * Connected to RangeLineEdit::selectionChanged.
     * Invoked whenever the QLineEdit's selection changes. We do not was normal selection behavior with this widget,
     * so it will automatically deselect any selection implicitly. Using keyPressEvents and right click context menu
     * operations, all behavior relying on selection can be done without messing with the state of the widget.
     */
    void selectionChangedEvent(){

        //Prevent selection by the user (Don't worry, this widget still lets you copy and paste properly with Ctrl+C and Ctrl+V)
        deselect();

    }

    /*
     * Copies the current text of this widget to the clipboard
     */
    void copyTextToClipboard(){

        QClipboard* clipboard = QGuiApplication::clipboard();
        if(clipboard != nullptr){

            clipboard->setText(text());

        }

    }

    /*
     * Pure virtual function that every subclass needs to define to allow Ctrl+C to work.
     * Production::Note: If a subclass has no needs for this behavior, just NOP it
     */
    virtual void copyValueToClipboard() = 0;

    /*
     * Pure virtual function that every subclass needs to define to allow Ctrl+V to work.
     * Production::Note: If a subclass has no needs for this behavior, just NOP it
     */
    virtual void pasteValueFromClipboard() = 0;

    /*
     * Helper slot to overcome Qt's inability to have a templated signal or slot.
     * This needs to be defined by the derived class and will essentially be a wrapper
     * for what would be equivalent to:
     *     signals:
     *         void valueChanged(ValueType value);
     */
    virtual void valueChangedPrivate() = 0;

    /*
     * Zeroes out all of the RangeInts
     */
    virtual void clearText(){

        int focusIndex = cursorPosition();

        foreach(::Range* range, m_ranges){

            if(range->rangeType() == "RangeInt"){

                static_cast<RangeInt*>(range)->m_value = 0;
                range->m_dirty = true;

            }

        }

        scrapeDirtiedRanges();

        setCursorPosition(focusIndex);

    }

public:

    QList<::Range*> m_ranges;

    //This determines if m_decimalRange should exist
    int       m_decimals;
    long long m_maxAllowableValue;
    int       m_prevCursorPosition;

    QPointer<TrianglePaintedButton> m_incrementButton;
    QPointer<TrianglePaintedButton> m_decrementButton;

    QPointer<QMenu> m_customContextMenu;
    QAction*        m_copyAsTextToClipBoardAction;
    QAction*        m_copyAsValueToClipBoardAction;
    QAction*        m_pasteAsValueFromClipBoardAction;
    QAction*        m_clearAction;

    RangeStringConstant* m_decimalString;
    RangeInt*            m_decimalRange;

    QColor m_highlightColor;

};

#endif // RANGELINEEDIT_H
