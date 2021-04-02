# RangeLineEdit
Qt QLineEdit subclass and formatting engine to create any type of restricted RegEx for numbers and characters that can be flexibly used. 
My example creates a Latitude and Longitude LineEdit validator.
This can technically be expanded for any use case, including a phone number validator, for example.

Hello! Welcome to the first fully functioning Latitude and Longitude DMS widget suite. 
This widget allows you to create editable QLineEdits that support user input, key press events, and mouse wheel events.

There is now also a DoubleLineEdit that allows you to manipulate a +/- decimal value with a standard dot notation,
much like Qt's QDoubleSpinBox, but has all of the features inherited from RangeLineEdit.

Usage:
```
#include "LatitudeLineEdit.h"

int decimalPrecision(4);
LatitudeLineEdit* latitudeLineEdit = new LatitudeLineEdit(nullptr, decimalPrecision);

double decimalDegree = 47.55;
latitudeLineEdit->setValue(decimalDegree);
//The widget now should display: N47°33'00.0000'' and would've emitted both
//a QLineEdit::TextChanged signal and a PositionalLineEdit::valueChanged signal.

double storedValue = latitudeLineEdit->value();

assert(std::fabs(decimalDegree - storedValue) < 0.000001);

//The above behavior fixes a common issue with any attempt to visualize a subset of a decimal value and the loss of precision resulting in such an operation.
//This widget manages floating point loss as a result of floating point multiplication and divsion and will keep track of said loss, even
//if not displayed within the widget itself. Once the user edits a field of the edit, the loss is cleared, but this keeps multiple values in sync when being
//populated by the same value at start.

//See MainWindow.cpp for more concrete examples and proofs of the widgets syncing properly from many contexts.

```

To make you own type, you need to subclass RangeLineEdit and give it a derived type of your choosing (Any custom class will work). Afterwards, there a handful of functions you HAVE to override in the subclass to make your type non-abstract. Generally, your subtype will have to convert your template parameterized value into something the Range subtypes can represent.
