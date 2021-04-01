#ifndef LONGITUDELINEEDIT_H
#define LONGITUDELINEEDIT_H

#include "PositionalLineEdit.h"

/*! class LongitudeLineEdit
 *
 * Derived type of PositionalLineEdit.
 * Sets up Range types that represent the DMS (Degree, Minute, Second)
 * representation of a longitudinal value.
 *
 */
class LongitudeLineEdit : public PositionalLineEdit{

public:

    /*
     * Value Constructor
     * @PARAM QWidget* parent   - Standard Qt parenting mechanism for memory management
     * @PARAM int      decimals - The amount of precision the user wishes to display (This does not affect stored precision)
     */
    LongitudeLineEdit(QWidget* parent = nullptr, int decimals = 2);

};

#endif // LONGITUDELINEEDIT_H
