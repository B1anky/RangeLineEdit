#include "PositionalLineEdit.h"
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

PositionalLineEdits::PositionalLineEdits(QWidget* parent)
    : QLineEdit                          (parent),
      m_ranges                           ({}),
      m_decimals                         (-1),
      m_undisplayedPrecision             (0.0),
      m_maxAllowableValue                (0),
      m_prevCursorPosition               (0),
      m_incrementButton                  (nullptr),
      m_decrementButton                  (nullptr),
      m_customContextMenu                (nullptr),
      m_copyAsTextToClipBoardAction      (nullptr),
      m_copyAsDecimalToClipBoardAction   (nullptr),
      m_pasteAsDecimalFromClipBoardAction(nullptr),
      m_clearAction                      (nullptr),
      m_degreeChar                       (nullptr),
      m_degreeInt                        (nullptr),
      m_degreeSymbol                     (nullptr),
      m_minuteInt                        (nullptr),
      m_minuteSymbol                     (nullptr),
      m_secondsInt                       (nullptr),
      m_secondSymbol                     (nullptr),
      m_decimalRange                     (nullptr),
      m_highlightColor                   (QColor(128, 128, 128, 75))
{

    setMouseTracking(true);
    setupIncrementAndDecrementButtons();
    createCustomContextMenu();

    connect(this, &PositionalLineEdits::cursorPositionChanged,      this, &PositionalLineEdits::cursorPositionChangedEvent, Qt::DirectConnection);
    connect(this, &PositionalLineEdits::selectionChanged,           this, &PositionalLineEdits::selectionChangedEvent,      Qt::DirectConnection);
    connect(this, &PositionalLineEdits::customContextMenuRequested, this, &PositionalLineEdits::showContextMenu,            Qt::DirectConnection);

}

PositionalLineEdits::~PositionalLineEdits(){

    clearCurrentValidators();

}

void PositionalLineEdits::setupIncrementAndDecrementButtons(){

    //Production::Note: It doesn't looks like the inverse character, but it appears properly
    m_incrementButton = new TrianglePaintedButton(TrianglePaintedButton::Direction::UP,   this);
    m_decrementButton = new TrianglePaintedButton(TrianglePaintedButton::Direction::DOWN, this);

    m_incrementButton->setMinimumWidth(15);
    m_decrementButton->setMinimumWidth(15);

    m_incrementButton->setMaximumWidth(25);
    m_decrementButton->setMaximumWidth(25);

    m_incrementButton->setMouseTracking(true);
    m_decrementButton->setMouseTracking(true);

    connect(m_incrementButton, &QPushButton::clicked, this, &PositionalLineEdits::increment, Qt::DirectConnection);
    connect(m_decrementButton, &QPushButton::clicked, this, &PositionalLineEdits::decrement, Qt::DirectConnection);

}

void PositionalLineEdits::createCustomContextMenu(){

    setContextMenuPolicy(Qt::CustomContextMenu);
    m_customContextMenu = new QMenu(this);

    m_copyAsTextToClipBoardAction       = m_customContextMenu->addAction("Copy  [As text]");
    m_copyAsDecimalToClipBoardAction    = m_customContextMenu->addAction("Copy  [As decimal]");
    m_pasteAsDecimalFromClipBoardAction = m_customContextMenu->addAction("Paste [From decimal]");
    m_clearAction                       = m_customContextMenu->addAction("Clear");

    connect(m_copyAsTextToClipBoardAction,       &QAction::triggered, this, &PositionalLineEdits::copyTextToClipboard,         Qt::DirectConnection);
    connect(m_copyAsDecimalToClipBoardAction,    &QAction::triggered, this, &PositionalLineEdits::copyDecimalToClipboard,      Qt::DirectConnection);
    connect(m_pasteAsDecimalFromClipBoardAction, &QAction::triggered, this, &PositionalLineEdits::pasteAsDecimalFromClipboard, Qt::DirectConnection);
    connect(m_clearAction,                       &QAction::triggered, this, &PositionalLineEdits::clearText,                   Qt::DirectConnection);

}

void PositionalLineEdits::copyTextToClipboard(){

    QClipboard* clipboard = QGuiApplication::clipboard();
    if(clipboard != nullptr){

        clipboard->setText(text());

    }

}

void PositionalLineEdits::copyDecimalToClipboard(){

    QClipboard* clipboard = QGuiApplication::clipboard();
    if(clipboard != nullptr){

        clipboard->setText(QString::number(textToDecimalValue(), 'f', m_decimals));

    }

}

void PositionalLineEdits::pasteAsDecimalFromClipboard(){

    QClipboard* clipboard = QGuiApplication::clipboard();
    if(clipboard != nullptr){

        bool isDecimal(false);
        double clipboardAsDouble = clipboard->text().toDouble(&isDecimal);
        if(isDecimal){

            setTextFromDecimalValue(clipboardAsDouble);

        }

    }

}

void PositionalLineEdits::clearText(){

    int focusIndex = cursorPosition();

    foreach(Range* range, m_ranges){

        if(range->rangeType() == "RangeInt"){

            static_cast<RangeInt*>(range)->m_value = 0;
            range->m_dirty = true;

        }

    }

    m_undisplayedPrecision = 0.0;

    scrapeDirtiedRanges();

    setCursorPosition(focusIndex);

}

void PositionalLineEdits::setPrecision(int decimals){

    if(m_decimals != decimals && decimals >= 0){

        m_decimals = decimals;
        int currentCursorPos = this->cursorPosition();

        //Generally this occurs if we're setting our type for the first time or changing our type dynamically
        if(m_decimalRange == nullptr){

            RangeStringConstant* decimalString = new RangeStringConstant(".");
            m_decimalRange = new RangeInt(m_decimals, std::pow(10, m_decimals) * 60 * 60);

            //We have to pop the " '' " seconds symbol off the back and move it to the right of the decimal ranges
            Range* secondSymbol = m_ranges.last();
            m_ranges.pop_back();
            m_ranges << decimalString << m_decimalRange << secondSymbol;

        }

        m_decimalRange->m_range = std::pow(10, m_decimals) - 1;

        QString curText = text();
        curText = curText.split(".").first();
        curText += "." + QString("0").repeated(m_decimals - m_decimalRange->valueLength()) + m_decimalRange->valueStr();

        syncRangeEdges();

        setCursorPosition(currentCursorPos);

    }

}

bool PositionalLineEdits::setValueForIndex(QString value, int index){

    bool valueWasSet = false;
    Range* range = getRangeForIndex(index);
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

double PositionalLineEdits::textToDecimalValue(){

    //Production::Note: This is what ensures we don't lose precision when scraping the decimal value
    return sumRangeInts() + m_undisplayedPrecision;

}

void PositionalLineEdits::setTextFromDecimalValue(double decimalDegrees){

    QChar signChar = decimalDegrees >= 0 ? m_degreeChar->m_positiveChar : m_degreeChar->m_negativeChar;
    m_degreeChar->m_value = signChar;

    decimalDegrees = std::fabs(decimalDegrees);

    if(decimalDegrees > m_maxAllowableValue){

        decimalDegrees = m_maxAllowableValue;

    }

    double degrees                    = std::floor(decimalDegrees);
    double minutesNotTruncated        = (decimalDegrees - degrees) * 60.0;
    double minutes                    = std::floor(minutesNotTruncated);
    double secondsNotTruncated        = (minutesNotTruncated - minutes) * 60.0;
    double seconds                    = std::floor(secondsNotTruncated);
    //Production::Note: m_decimalRange->m_range == std::pow(10, m_decimals) - 1, so we need to divide cleanly by 10, 100, 1000, etc., not 9, 99, 999
    double decimalSecondsNotTruncated = (secondsNotTruncated - seconds) * (m_decimalRange->m_range + 1.0);
    double decimalSeconds             = std::floor(decimalSecondsNotTruncated);

    minutes        = std::floor(minutes);
    seconds        = std::floor(seconds);
    decimalSeconds = std::floor(decimalSeconds);

    m_degreeInt->m_value    = degrees;
    m_minuteInt->m_value    = minutes;
    m_secondsInt->m_value   = seconds;
    m_decimalRange->m_value = decimalSeconds;
    m_undisplayedPrecision  = decimalDegrees - sumRangeInts();

    //Production::Note: The below episolon scales with how many decimals are being used, so it's completely dynamic
    const double minimumDecimalValue(1.0 / (3600.0 * (m_decimalRange->m_range + 1.0)));
    const double epsilonErrorAllowed(minimumDecimalValue / 10.0);

    if(m_undisplayedPrecision > epsilonErrorAllowed){

        //Production::Note: If we found enough error to be greater than the epsilon,
        //that implies we SHOULD have incremented one more time,
        //but because doubles and floats are sometimes imprecise during arithmetic operations with certain values,
        //it erroneously floored an extra decimal value.
        //If we can increment the decimal range it should fix up any visual desync occurring from being off by 1 decimal.
        if(m_decimalRange->increment(0)){

            //We also need to remove from the undisplayed precision the amount we were able to increment by (Brings is closer to 0)
            m_undisplayedPrecision -= minimumDecimalValue;

        }

    }

    syncRangeSigns();
    maximumExceededFixup();

}

void PositionalLineEdits::setActiveIndexHighlightColor(const QColor& highlightColor, bool implicitlyMakeSemiTransparent){

    if(highlightColor.isValid()){

        m_highlightColor = highlightColor;

        //This will make it so a user could, for example use Qt::red, Qt::blue, Qt::green, etc.,
        //and it'll automatically make it semi-transparent without multiple lines of code on their part
        if(implicitlyMakeSemiTransparent){

            m_highlightColor.setAlpha(75);

        }

    }

}

Range* PositionalLineEdits::findAdjacentNonStringConstantRange(Range* range, bool seekLeftRange){

    Range* adjacentRange(nullptr);
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

Range* PositionalLineEdits::getRangeForIndex(int index){

    Range* rangeForIndex(nullptr);
    foreach(Range* range, m_ranges){

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

void PositionalLineEdits::clearCurrentValidators(){

    foreach(Range* range, m_ranges){

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

    //Ensures everything is properly not pointing to deleted memory
    m_degreeChar   = nullptr;
    m_degreeInt    = nullptr;
    m_degreeSymbol = nullptr;
    m_minuteInt    = nullptr;
    m_minuteSymbol = nullptr;
    m_secondsInt   = nullptr;
    m_secondSymbol = nullptr;

}

void PositionalLineEdits::syncRangeEdges(){

    int curRangeOffset(0);

    for(int i = 0; i < m_ranges.size() - 1; ++i){

        Range* leftRange  = m_ranges[i];
        Range* rightRange = m_ranges[i + 1];

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

    scrapeDirtiedRanges();

}

void PositionalLineEdits::increment(){    

    //Check if this position belongs to a valid Range
    Range* range = getRangeForIndex(m_prevCursorPosition);

    if(range != nullptr){

        int localRangeIndex = range->m_charIndexEnd - m_prevCursorPosition;

        if(range->rangeType() != "RangeStringConstant"){

            range->increment(localRangeIndex);

            syncRangeSigns();
            maximumExceededFixup();
            scrapeDirtiedRanges();

            setCursorPosition(m_prevCursorPosition);

        }

    }

}

void PositionalLineEdits::decrement(){       

    //Check if this position belongs to a valid Range
    Range* range = getRangeForIndex(m_prevCursorPosition);

    if(range != nullptr){

        int localRangeIndex = range->m_charIndexEnd - m_prevCursorPosition;

        if(range->rangeType() != "RangeStringConstant"){

            range->decrement(localRangeIndex);

            syncRangeSigns();
            maximumExceededFixup();
            scrapeDirtiedRanges();

            setCursorPosition(m_prevCursorPosition);

        }

    }

}

void PositionalLineEdits::seekLeft(){

    int focusIndex = this->cursorPosition();

    //No point to move left if we're at 0
    if(focusIndex > 0){

        //Assume this is valid and prove otherwise below
        int newCursorPosition = focusIndex - 1;

        //Check which Range belongs to the current cursor position
        Range* range = getRangeForIndex(focusIndex);

        if(range != nullptr){

            if(newCursorPosition < range->m_charIndexStart || range->rangeType() == "RangeStringConstant"){

                Range* leftMostAdjacentRangeValue = findAdjacentNonStringConstantRange(range, true);

                //Grab the range to its left's end index to hop properly
                if(leftMostAdjacentRangeValue != nullptr){

                    newCursorPosition = leftMostAdjacentRangeValue->m_charIndexEnd;

                }

            }

        }

        setCursorPosition(newCursorPosition);

    }

}

void PositionalLineEdits::seekRight(){

    int focusIndex = this->cursorPosition();

    //No point to move right if we're at the end
    if(focusIndex < this->text().length()){

        //Assume this is valid and prove otherwise below
        int newCursorPosition = focusIndex + 1;

        //Check which Range belongs to the current cursor position
        Range* range = getRangeForIndex(focusIndex);

        if(range != nullptr){

            if(newCursorPosition > range->m_charIndexEnd || range->rangeType() == "RangeStringConstant"){

                Range* rightMostAdjacentRangeValue = findAdjacentNonStringConstantRange(range, false);

                //Grab the range to its right's start index to hop properly
                if(rightMostAdjacentRangeValue != nullptr){

                    newCursorPosition = rightMostAdjacentRangeValue->m_charIndexStart;

                }

            }

            setCursorPosition(newCursorPosition);

        }

    }

}

void PositionalLineEdits::maximumExceededFixup(){

    //Assume false and prove otherwise
    bool atOrExceedsValue(false);

    //This will essentially zero out all RangeInts and set the first one to the max allowed value
    if(fabs(textToDecimalValue()) >= m_maxAllowableValue){

        atOrExceedsValue = true;
        foreach(Range* range, m_ranges){

            if(range->rangeType() == "RangeInt"){

                static_cast<RangeInt*>(range)->m_value = 0;
                range->m_dirty = true;

            }

        }

    }

    if(Q_UNLIKELY(atOrExceedsValue)){

        //Look for the first RangeInt and set it to the maximum range
        foreach(Range* range, m_ranges){

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
        scrapeDirtiedRanges();

        setCursorPosition(focusIndex);

    }

}

double PositionalLineEdits::sumRangeInts(){

    double sum(0.0);

    foreach(Range* range, m_ranges){

        if(range->rangeType() == "RangeInt"){

            sum += static_cast<RangeInt*>(range)->m_value / static_cast<double>(range->divisor());

        }

    }

    return sum;

}

void PositionalLineEdits::syncRangeSigns(){

    //Assume it to be positive
    bool charSign(true);

    //Grab the first RangeChar's sign
    if( (m_ranges.isEmpty() == false) && (m_ranges.first()->rangeType() == "RangeChar") ){

        RangeChar* rangeChar = static_cast<RangeChar*>(m_ranges.first());

        charSign = (rangeChar->valueStr() == rangeChar->m_positiveChar);

    }

    //Now loop through all RangeInts to make them the same sign (All positive or all negative)
    foreach(Range* range, m_ranges){

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

void PositionalLineEdits::scrapeTextFromRangeValue(Range* range, bool overrideBeingDirty){

    if(range->m_dirty || overrideBeingDirty){

        QString curText    = this->text();
        QString paddedText = range->valueStr();
        curText = curText.replace(range->m_charIndexStart, paddedText.length(), paddedText);

        range->m_dirty = false;

        //Production::Note: All calls to PositionalLineEdits::scrapeTextFromRangeValue(...) are wrapped in
        //blockSignals(true) to prevent multiple emissions of textChanged per batch update.
        //At the end of the block we check to see if our original text no longer matches the "new"
        //scraped text and manually emit QLineEdit::textChanged(this->text()) once after unblocking signals
        setText(curText);

    }

}

void PositionalLineEdits::scrapeDirtiedRanges(bool overrideBeingDirty){

    QString originalText = text();

    blockSignals(true);

    foreach(Range* range, m_ranges){

        scrapeTextFromRangeValue(range, overrideBeingDirty);

    }

    blockSignals(false);

    //Doing this will ensure one emission of QLineEdit::textChanged(...) will occur for a given batch update
    if(originalText != text()){

        emit textChanged(text());

    }

}

void PositionalLineEdits::keyPressEvent(QKeyEvent* keyEvent){

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

        if(setValueForIndex("0", this->cursorPosition())){

            m_undisplayedPrecision = 0.0;

        }

        seekLeft();

    }else if(key == Qt::Key_Delete){

        if(setValueForIndex("0", this->cursorPosition())){

            m_undisplayedPrecision = 0.0;

        }

        seekRight();

    }else if(keyEvent->matches(QKeySequence::Copy)){

        copyDecimalToClipboard();

    }else if(keyEvent->matches(QKeySequence::Paste)){

        pasteAsDecimalFromClipboard();

    }else if(key == Qt::Key_Home || key == Qt::Key_End){

        QLineEdit::keyPressEvent(keyEvent);

    }
    //Production::Note: Add new `else if`s above this one
    else if(keyEvent->text().length() == 1){

        if(setValueForIndex(keyEvent->text(), this->cursorPosition())){

            seekRight();

        }

    }//Production::Note: Don't even think about adding another `else if` below here

}

void PositionalLineEdits::focusInEvent(QFocusEvent* focusEvent){

    int focusIndex = cursorPosition();

    //This serves to make sure if the user somehow improperly called setText from another context and didn't use the supplied
    //PositionalLineEdits::setTextFromDecimalValue(...), that we will clear the text and reconstruct the string on focusIn.
    clear();
    scrapeDirtiedRanges(true);

    setCursorPosition(focusIndex);

    QLineEdit::focusOutEvent(focusEvent);

}

void PositionalLineEdits::focusOutEvent(QFocusEvent* focusEvent){

    int focusIndex = cursorPosition();

    //This serves to make sure if the user somehow improperly called setText from another context and didn't use the supplied
    //PositionalLineEdits::setTextFromDecimalValue(...), that we will clear the text and reconstruct the string on focusOut.
    clear();
    scrapeDirtiedRanges(true);

    setCursorPosition(focusIndex);

    QLineEdit::focusOutEvent(focusEvent);

}

void PositionalLineEdits::paintEvent(QPaintEvent* paintEvent){

    //Draw the QLineEdit as normal
    QLineEdit::paintEvent(paintEvent);

    //Below highlights the current text that has focus in the widget and will be affected by an increment, decrement, or key press operation
    if( (this->hasFocus() || m_incrementButton->underMouse() || m_decrementButton->underMouse()) && cursorPosition() < text().length() - 1){

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

void PositionalLineEdits::resizeEvent(QResizeEvent* resizeEvent){

    //This will make it so the buttons have some wiggle room (simulates as if they're in a layout, without actually being in one)
    QSize buttonSize(width() * 0.10, height() / 2 + 1);

    //These buttons have an enforced minimum and maximum size, so it will scale slightly with the size of the widget, to an extent
    m_incrementButton->resize(buttonSize);
    m_decrementButton->resize(buttonSize);

    m_incrementButton->move(width() - m_incrementButton->width(), 0);
    m_decrementButton->move(width() - m_decrementButton->width(), height() / 2);

    QLineEdit::resizeEvent(resizeEvent);

}

void PositionalLineEdits::wheelEvent(QWheelEvent* wheelEvent){

    if(this->hasFocus()){

        if(wheelEvent->angleDelta().y() > 0){

            increment();

        }else{

            decrement();

        }

    }

    wheelEvent->accept();

}

void PositionalLineEdits::showContextMenu(const QPoint& pos){

    //Optionally enable / disable the paste operation depending on whether the clipboard actually holds a valid decimal string
    if(QGuiApplication::clipboard() != nullptr){

        bool canConvertToDecimal(false);
        QGuiApplication::clipboard()->text().toDouble(&canConvertToDecimal);
        m_pasteAsDecimalFromClipBoardAction->setEnabled(canConvertToDecimal);

    }

    m_customContextMenu->exec(mapToGlobal(pos));

}

void PositionalLineEdits::cursorPositionChangedEvent(int, int cur){

    Range* range = getRangeForIndex(cur);
    if(range != nullptr && range->rangeType() == "RangeStringConstant"){

        //Go to the left, if possible, otherwise fall back to the right
        if(range->m_leftRange != nullptr){

            setCursorPosition(range->m_leftRange->m_charIndexEnd);

        }else if(range->m_rightRange != nullptr){

            setCursorPosition(range->m_rightRange->m_charIndexStart);

        }

    }

    if(this->hasFocus()){

        m_prevCursorPosition = cursorPosition();

    }

}

void PositionalLineEdits::selectionChangedEvent(){

    //Prevent selection by the user (Don't worry, this widget still lets you copy and paste properly with Ctr+c and Ctrl+v)
    deselect();

}
