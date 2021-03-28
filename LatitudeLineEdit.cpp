#include "LatitudeLineEdit.h"

LatitudeLineEdit::LatitudeLineEdit(int decimals, QWidget* parent)
    : PositionalLineEdits(parent)
{

    m_type = PositionalLineEdits::Type::LATITUDE;
    m_degreeChar   = new RangeChar('S', 'N');
    m_degreeInt    = new RangeInt(90, 1);
    m_degreeSymbol = new RangeStringConstant("°");
    m_minuteInt    = new RangeInt(59, 60);
    m_minuteSymbol = new RangeStringConstant("'");
    m_secondsInt   = new RangeInt(59, 3600);
    m_secondSymbol = new RangeStringConstant("''");

    m_ranges << m_degreeChar << m_degreeInt << m_degreeSymbol << m_minuteInt << m_minuteSymbol << m_secondsInt << m_secondSymbol;
    m_prevCursorPosition = 0;
    syncRangeEdges();

    m_maxAllowableValue = 90;
    setPrecision(decimals);

    setCursorPosition(0);

}
