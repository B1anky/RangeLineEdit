#ifndef LATITUDELINEEDIT_H
#define LATITUDELINEEDIT_H

#include "PositionalLineEdit.h"

/*! class LatitudeLineEdit
 *
 * Derived type of PositionalLineEdit.
 * Sets up Range types that represent the DMS (Degree, Minute, Second)
 * representation of a latitudinal value.
 *
 */
class LatitudeLineEdit : public PositionalLineEdit{

public:

    /*
     * Value Constructor
     * @PARAM QWidget* parent   - Standard Qt parenting mechanism for memory management
     * @PARAM int      decimals - The amount of precision the user wishes to display (This does not affect stored precision)
     */
    LatitudeLineEdit(QWidget* parent = nullptr, int decimals = 2);

};

#endif // LATITUDELINEEDIT_H
