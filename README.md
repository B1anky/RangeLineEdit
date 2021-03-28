# PositionalLineEdit
Qt QLineEdit subclass and formatting engine to create any type of restricted RegEx for numbers and characters that can be flexibly used. 
My example creates a Latitude and Longitude LineEdit validator.
This can technically be expanded for any use case, including a phone number validator, for example.

Hello! Welcome to the first fully functioning Latitude and Longitude DMS widget suite. 
This widget allows you to create editable QLineEdits that support user input, key press events, and mouse wheel events.

Usage:
```
#include "LatitudeLineEdit.h"

int decimalPrecision(4);
LatitudeLineEdit* latitudeLineEdit = new LatitudeLineEdit(decimalPrecision);

double decimalDegree = 47.55;
latitudeLineEdit->setTextFromDecimalValue(decimalDegree);
//The widget now should display: N47°33'00.00000''

double storedValue = latitudeLineEdit->textToDecimalValue();

assert(std::fabs(decimalDegree - storedValue) < 0.000001);

//The above behavior fixes a common issue with any attempt to visualize a subset of a decimal value and the loss of precision resulting in such an operation.
//This widget manages floating point loss as a result of floating point multiplication and divsion and will keep track of said loss, even
//if not displayed within the widget itself.

//See MainWindow.cpp for more concrete examples and proofs of the widgets syncing properly from many contexts.

```
