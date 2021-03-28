#include "PositionalLineEdits.h"
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
    : QLineEdit                    (parent),
      m_type                       (INVALID),
      m_ranges                     ({}),
      m_decimals                   (-1),
      m_decimalRange               (nullptr),
      m_undisplayedPrecision       (0.0),
      m_maxAllowableValue          (0),
      m_showFocusFromButtonHovering(false),
      m_prevCursorPosition         (0),
      m_incrementButton            (nullptr),
      m_decrementButton            (nullptr),
      m_customContextMenu          (nullptr),
      m_degreeChar                 (nullptr),
      m_degreeInt                  (nullptr),
      m_degreeSymbol               (nullptr),
      m_minuteInt                  (nullptr),
      m_minuteSymbol               (nullptr),
      m_secondsInt                 (nullptr),
      m_secondSymbol               (nullptr)
{
    setMouseTracking(true);
    setupIncrementAndDecrementButtons();
    createCustomContextMenu();
    connect(this, &PositionalLineEdits::cursorPositionChanged,      this, &PositionalLineEdits::cursorPositionChangedEvent, Qt::DirectConnection);
    connect(this, &PositionalLineEdits::selectionChanged,           this, &PositionalLineEdits::selectionChangedEvent,      Qt::DirectConnection);
    connect(this, &PositionalLineEdits::customContextMenuRequested, this, &PositionalLineEdits::showContextMenu,            Qt::DirectConnection);
}

PositionalLineEdits::~PositionalLineEdits()
{
    clearCurrentValidators();
}

void PositionalLineEdits::setupIncrementAndDecrementButtons(){

    //Production::Note: It doesn't looks like the inverse character, but it appears properly
    m_incrementButton = new TrianglePaintedButton(TrianglePaintedButton::Direction::UP,   this);
    m_decrementButton = new TrianglePaintedButton(TrianglePaintedButton::Direction::DOWN, this);

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

    QAction* copyAsTextToClipBoardAction    = m_customContextMenu->addAction("Copy [text]");
    QAction* copyAsDecimalToClipBoardAction = m_customContextMenu->addAction("Copy [decimal]");
    QAction* clearAction                    = m_customContextMenu->addAction("Clear");

    connect(copyAsTextToClipBoardAction,    &QAction::triggered, this, &PositionalLineEdits::copyTextToClipboard,    Qt::DirectConnection);
    connect(copyAsDecimalToClipBoardAction, &QAction::triggered, this, &PositionalLineEdits::copyDecimalToClipboard, Qt::DirectConnection);
    connect(clearAction,                    &QAction::triggered, this, &PositionalLineEdits::clearText,              Qt::DirectConnection);

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

        clipboard->setText(QString::number(sumRangeInts(), 'f', m_decimals));

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

void PositionalLineEdits::setPrecision(int decimals)
{

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

bool PositionalLineEdits::setValueForIndex(QString value, int index)
{

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

double PositionalLineEdits::textToDecimalValue()
{

    return sumRangeInts() + m_undisplayedPrecision;

}

void PositionalLineEdits::setTextFromDecimalValue(double decimalDegrees)
{

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

    //Production::Note: The below episolon scaled with how many decimals are being used
    const double minimumDecimalValue(1.0 / (3600.0 * (m_decimalRange->m_range + 1.0)));
    const double epsilonErrorAllowed(minimumDecimalValue / 10.0);

    if(m_undisplayedPrecision > epsilonErrorAllowed){

        if(m_decimalRange->increment(0)){

            m_undisplayedPrecision -= minimumDecimalValue;

        }

    }

    syncRangeSigns();
    maximumExceededFixup();

}

Range* PositionalLineEdits::findAdjacentNonStringConstantRange(Range* range, bool leftRange)
{

    Range* adjacentRange(nullptr);
    int iterInd(0);

    //Seek left
    if(leftRange){

        iterInd = m_ranges.indexOf(range) - 1;

        while(iterInd > 0){

            if(m_ranges.at(iterInd)->rangeType() != "RangeStringConstant"){
                adjacentRange = m_ranges.at(iterInd);
                break;
            }

            --iterInd;

        }

    }
    //Seek right
    else{

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

void PositionalLineEdits::clearCurrentValidators()
{

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

    //Should've been deleted in the above loop, but let's make sure anyway
    m_decimalRange = nullptr;

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

void PositionalLineEdits::increment()
{
    //int focusIndex = this->cursorPosition();
    int focusIndex = m_prevCursorPosition;

    //Check if this the position that belongs to a Range
    Range* range = getRangeForIndex(focusIndex);
    int localRangeIndex = range->m_charIndexEnd - focusIndex;

    if(range != nullptr && (range->rangeType() != "RangeStringConstant") ){

        range->increment(localRangeIndex);
        syncRangeSigns();
        maximumExceededFixup();
        scrapeDirtiedRanges();
        setCursorPosition(focusIndex);

    }

}

void PositionalLineEdits::decrement()
{
    int focusIndex = m_prevCursorPosition;

    //Check if this the position that belongs to a Range
    Range* range = getRangeForIndex(focusIndex);
    int localRangeIndex = range->m_charIndexEnd - focusIndex;

    if(range != nullptr && (range->rangeType() != "RangeStringConstant") ){

        range->decrement(localRangeIndex);
        syncRangeSigns();
        maximumExceededFixup();
        scrapeDirtiedRanges();
        setCursorPosition(focusIndex);

    }

}

void PositionalLineEdits::seekLeft()
{

    int focusIndex = this->cursorPosition();

    //No point to move left if we're at 0
    if(focusIndex > 0){

        //Assume this is valid and prove otherwise below
        int newCursorPosition = focusIndex - 1;

        //Check which Range belongs to the current cursor positions
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

void PositionalLineEdits::seekRight()
{

    int focusIndex = this->cursorPosition();

    //No point to move right if we're at the end
    if(focusIndex < this->text().length()){

        //Check if this the position that belongs to a Range
        Range* range = getRangeForIndex(focusIndex);

        if(range != nullptr){

            //Assume this is valid
            int newCursorPosition = focusIndex + 1;

            if(newCursorPosition > range->m_charIndexEnd || range->rangeType() == "RangeStringConstant"){

                Range* rightMostAdjacentRangeValue = findAdjacentNonStringConstantRange(range, false);

                //Grab the range to its right's start index to hop properly
                if(rightMostAdjacentRangeValue != nullptr){

                    newCursorPosition = rightMostAdjacentRangeValue->m_charIndexStart;

                }else{

                    newCursorPosition = focusIndex;

                }

            }

            setCursorPosition(newCursorPosition);

        }

    }

}

void PositionalLineEdits::maximumExceededFixup()
{

    //Simply just zero it all out
    bool atOrExceedsValue(false);
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

        int focusIndex = this->cursorPosition();
        syncRangeSigns();
        scrapeDirtiedRanges();
        setCursorPosition(focusIndex);

    }

}

double PositionalLineEdits::sumRangeInts()
{

    double sum(0.0);

    foreach(Range* range, m_ranges){

        if(range->rangeType() == "RangeInt"){

            sum += static_cast<RangeInt*>(range)->m_value / static_cast<double>(range->divisor());

        }

    }

    return sum;

}

void PositionalLineEdits::syncRangeSigns()
{

    //Assume it to be positive
    bool sign = true;

    //Grab the first RangeChar's sign
    if(m_ranges.isEmpty() == false && m_ranges.first()->rangeType() == "RangeChar"){

        RangeChar* rangeChar = static_cast<RangeChar*>(m_ranges.first());

        if(rangeChar->valueStr() == rangeChar->m_positiveChar){

            sign = true;

        }else if(rangeChar->valueStr() == rangeChar->m_negativeChar){

            sign = false;

        }

    }

    //Now loop through all RangeInts to make them the same sign
    foreach(Range* range, m_ranges){

        if(range->rangeType() == "RangeInt"){

            RangeInt* rangeInt = static_cast<RangeInt*>(range);

            bool rangeSign = rangeInt->m_value >= 0;

            if(rangeSign != sign){

                rangeInt->m_value *= -1;
                rangeInt->m_dirty = true;

            }

        }

    }

    int focusIndex = this->cursorPosition();
    scrapeDirtiedRanges(true);
    setCursorPosition(focusIndex);

}

void PositionalLineEdits::scrapeTextFromRangeValue(Range* range, bool overrideBeingDirty){

    if(range->m_dirty || overrideBeingDirty){

        //Set the string based on the value, padded by 0s where the value doesn't reach the length
        QString curText    = this->text();
        QString paddedText = range->valueStr();
        curText = curText.replace(range->m_charIndexStart, range->rangeLength(), paddedText);

        range->m_dirty = false;
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

    if(originalText != text()){
        emit textChanged(text());
    }

}

void PositionalLineEdits::keyPressEvent(QKeyEvent* keyEvent)
{

    int key = keyEvent->key();
    int focusIndex = this->cursorPosition();

    if(key == Qt::Key_Up){

        increment();

    }else if(key == Qt::Key_Down){

        decrement();

    }else if(key == Qt::Key_Left){

        seekLeft();

    }else if(key == Qt::Key_Right){

        seekRight();

    }else if(key == Qt::Key_Backspace){

        setValueForIndex("0", focusIndex);
        seekLeft();

    }else if(key == Qt::Key_Delete){

        setValueForIndex("0", focusIndex);
        seekRight();

    }else if(keyEvent->text().size() == 1){

        if(setValueForIndex(keyEvent->text(), focusIndex)){

            seekRight();

        }

    }else if(key == Qt::Key_Home || key == Qt::Key_End){

        QLineEdit::keyPressEvent(keyEvent);

    }

}


void PositionalLineEdits::focusOutEvent(QFocusEvent* focusEvent)
{

    int focusIndex = cursorPosition();
    clear();
    scrapeDirtiedRanges(true);
    setCursorPosition(focusIndex);

    QLineEdit::focusOutEvent(focusEvent);

}
void PositionalLineEdits::paintEvent(QPaintEvent* paintEvent)
{

    QLineEdit::paintEvent(paintEvent);


    //Below highlights the current text that has focus in the widget and will be affected by an increment, decrement, or key press operation
    if( (this->hasFocus() || m_incrementButton->hasFocus() || m_decrementButton->hasFocus()) && cursorPosition() < text().length() - 1){

        QPainter painter(this);
        painter.setPen(QPen(QColor(255, 255, 255, 0)));
        painter.setBrush(QBrush(QColor(128, 128, 128, 75)));

        QFontMetrics fontMetrics(font());

        int pixelsWide = fontMetrics.width(text().at(cursorPosition()));
        int pixelsHigh = fontMetrics.height();

        QPoint topLeft  = cursorRect().topLeft();
        //The topLeft start width() is too far from the blinking caret (text cursor), so this offsets the start properly
        topLeft.setX(topLeft.x() + cursorRect().width() / 2);
        QPoint botRight = QPoint(topLeft.x() + pixelsWide - 1, topLeft.y() + pixelsHigh);

        painter.drawRect(QRect(topLeft, botRight));

    }


}

void PositionalLineEdits::resizeEvent(QResizeEvent *resizeEvent)
{

    QSize buttonSize(width() * 0.10, height() / 2 + 1);

    m_incrementButton->resize(buttonSize);
    m_decrementButton->resize(buttonSize);

    m_incrementButton->move(width() - m_incrementButton->width(), 0);
    m_decrementButton->move(width() - m_decrementButton->width(), height() / 2);

    QLineEdit::resizeEvent(resizeEvent);

}

void PositionalLineEdits::wheelEvent(QWheelEvent* wheelEvent)
{

    if(this->hasFocus()){

        if(wheelEvent->delta() > 0){

            increment();

        }else{

            decrement();

        }

    }

    wheelEvent->accept();

}

void PositionalLineEdits::showContextMenu(const QPoint &pos)
{

     m_customContextMenu->exec(mapToGlobal(pos));
}

void PositionalLineEdits::cursorPositionChangedEvent(int, int cur)
{

    Range* range = getRangeForIndex(cur);
    if(range != nullptr && range->rangeType() == "RangeStringConstant"){

        //go to the left by default, if possible, else right
        if(range->m_leftRange != nullptr){

            setCursorPosition(range->m_leftRange->m_charIndexEnd);

        }

    }

    if(this->hasFocus()){

        m_prevCursorPosition = cursorPosition();

    }

}

void PositionalLineEdits::selectionChangedEvent()
{

    //Prevent selection
    deselect();

}

RangeInt::RangeInt(int range, int divisor)
    : Range    (),
      m_range  (range),
      m_value  (0),
      m_divisor(divisor)
{
    //NOP
}

void RangeInt::setRange(int range)
{
    m_range = range;
}

void RangeInt::setDivisor(int divisor)
{
    if(divisor > 0){

        m_divisor = divisor;

    }else{

        std::cerr << "Error. RangeInt::setDivisor(...) called with a divisor less than or equal to 0." << std::endl;

    }
}

bool RangeInt::increment(int index){

    //The index refers to the indexing from right to left (not what people are used to)
    //i.e. for dgerees it has format [000, 180], so in this example for 180
    //index == 0 = 0
    //index == 1 = 8
    //index == 2 = 1

    int valueToIncrementBy = std::pow(10, index);
    int originalValue      = m_value;
    //Incrementing a positive number
    if(m_value > 0){

        if(m_value + valueToIncrementBy >= m_range + 1){

            if(m_leftRange){

                //If we can increment all of our left Ranges
                if(m_leftRange->increment(0)){

                    m_value += valueToIncrementBy - m_range - 1;

                }
                //We've hit our maximum value if we couldn't increment recursively
                else{

                    m_value = m_range;

                }

            }

        }else{

            m_value += valueToIncrementBy;

        }

    }
    //Incrementing a negative number (Should make the number approach 0 (i.e. -20 + 10 = -10)
    else if(m_value < 0){

        //If we increment our value above 0, but we're supposed to be negative (i.e. -05 + 10 = -55 && increment the left)
        if(m_value + valueToIncrementBy > 0){



            bool flippingFromNegToPos = allValuesToLeftAreZero() && leftMostRange()->increment(0);

            if(flippingFromNegToPos){

                m_value = -m_value;

            }else if(m_leftRange != nullptr && m_leftRange->increment(0)){

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

                //We're actually incrementing what should be a negative number is in this case
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

    //The index refers to the indexing from right to left (not what people are used to)
    //i.e. for dgerees it has format [000, 180], so in this example for 180
    //index == 0 = 0
    //index == 1 = 8
    //index == 2 = 1

    int valueToDecrementBy = std::pow(10, index);
    int originalValue      = m_value;

    //Decrementing a positive number
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
    //Decrementing a negative number
    else if(m_value < 0){

        //If we underflow (i.e. At -55 minutes and we want to subtract 10, we went below -59)
        //Expect to become -05, but we have to be able to borrow from our left
        if(m_value - valueToDecrementBy < -m_range){

            //We have to have a left range
            if(m_leftRange != this){

                //If we can decrement the left range
                if(m_leftRange->decrement(0)){

                    //(i.e. -55 - 10 + 59 + 1 = -65 + 60 = -5)
                    m_value = m_value - valueToDecrementBy + m_range + 1;

                }

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

int RangeInt::valueLength()
{
    return QString::number(abs(m_value)).length();
}

int RangeInt::rangeLength()
{
    return QString::number(m_range).length();
}

QString RangeInt::valueStr()
{
    return QString("0").repeated(rangeLength() - valueLength()) + QString::number(abs(m_value));
}

QString RangeInt::rangeType()
{
    return "RangeInt";
}

int RangeInt::divisor()
{
    return m_divisor;
}

bool RangeInt::setValueForIndex(const QString& value, int index)
{

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

bool RangeChar::increment(int)
{
    bool incremented(false);
    if(m_value != m_positiveChar){

        incremented = true;
        m_value = m_positiveChar;
        m_dirty = true;

    }

    return incremented;
}

bool RangeChar::decrement(int)
{
    bool decremented(false);
    if(m_value != m_negativeChar){
        m_value = m_negativeChar;
        decremented = true;
        m_dirty = true;

    }

    return decremented;
}

int RangeChar::valueLength()
{
    return 1;
}

int RangeChar::rangeLength()
{
    return 1;
}

QString RangeChar::valueStr()
{
    return QString(m_value);
}

QString RangeChar::rangeType()
{
    return "RangeChar";
}

int RangeChar::divisor()
{
    return 1;
}

bool RangeChar::setValueForIndex(const QString& value, int index)
{

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

void RangeChar::setRange(QChar negativeChar, QChar positiveChar)
{
    m_negativeChar = negativeChar;
    m_positiveChar = positiveChar;
}

RangeChar::RangeChar(QChar negativeChar, QChar positiveChar)
    : Range(),
      m_negativeChar(negativeChar),
      m_positiveChar(positiveChar),
      m_value       (m_positiveChar)
{
    //NOP
}

Range::Range()
    : m_charIndexStart(0),
      m_charIndexEnd  (0),
      m_leftRange     (nullptr),
      m_rightRange    (nullptr),
      m_dirty         (false)
{
    //NOP
}

Range::~Range(){
    m_leftRange  = nullptr;
    m_rightRange = nullptr;
}

Range* Range::leftMostRange()
{
    Range* range = this;
    while(range != nullptr && range->m_leftRange != nullptr){

        range = range->m_leftRange;

    }

    return range;
}

RangeInt* Range::leftMostRangeInt()
{
    RangeInt* rangeInt = dynamic_cast<RangeInt*>(this);
    Range* rangeIter = this->m_leftRange;
    while(rangeIter != nullptr){

        //Production::Note: We don't break because we might find another, more left RangeInt
        if(rangeIter->rangeType() == "RangeInt"){

            rangeInt = static_cast<RangeInt*>(rangeIter);

        }

        rangeIter = rangeIter->m_leftRange;

    }

    return rangeInt;
}

bool Range::allValuesToLeftAreZero()
{
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

bool Range::leftMostRangeCharSign()
{
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

RangeStringConstant::RangeStringConstant(QString stringPlaceHolder)
    : Range  (),
      m_value(stringPlaceHolder)
{
    //NOP
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

int RangeStringConstant::valueLength()
{
    return m_value.length();
}

int RangeStringConstant::rangeLength()
{
    return m_value.length();
}

QString RangeStringConstant::valueStr()
{
    return m_value;
}

QString RangeStringConstant::rangeType()
{
    return "RangeStringConstant";
}

int RangeStringConstant::divisor()
{
    return 1;
}

bool RangeStringConstant::setValueForIndex(const QString& value, int index)
{
    Q_UNUSED(value);
    Q_UNUSED(index);
    return true;
}
