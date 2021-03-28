#include "MainWindow.h"
#include "LatitudeLineEdit.h"
#include "LongitudeLineEdit.h"
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{

    QWidget* centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    QVBoxLayout* centralVLayout = new QVBoxLayout;
    centralWidget->setLayout(centralVLayout);

    QHBoxLayout*       lineEditLayout    = new QHBoxLayout;
    LatitudeLineEdit*  latitudeLineEdit  = new LatitudeLineEdit(2);
    LongitudeLineEdit* longitudeLineEdit = new LongitudeLineEdit(2);
    lineEditLayout->addWidget(latitudeLineEdit);
    lineEditLayout->addWidget(longitudeLineEdit);
    centralVLayout->addLayout(lineEditLayout);

    QHBoxLayout*       lineEditLayoutError    = new QHBoxLayout;
    LatitudeLineEdit*  latitudeLineEditError  = new LatitudeLineEdit(2);
    LongitudeLineEdit* longitudeLineEditError = new LongitudeLineEdit(2);
    lineEditLayoutError->addWidget(latitudeLineEditError);
    lineEditLayoutError->addWidget(longitudeLineEditError);
    centralVLayout     ->addLayout(lineEditLayoutError);

    QHBoxLayout* decimalLabelLayout             = new QHBoxLayout;
    QLabel*      latitudeDecimalLabel           = new QLabel;
    QLabel*      latitudeDecimalLabelSecondary  = new QLabel;
    QLabel*      longitudeDecimalLabel          = new QLabel;
    QLabel*      longitudeDecimalLabelSecondary = new QLabel;

    decimalLabelLayout->addWidget(latitudeDecimalLabel);
    decimalLabelLayout->addWidget(latitudeDecimalLabelSecondary);
    decimalLabelLayout->addWidget(longitudeDecimalLabel);
    decimalLabelLayout->addWidget(longitudeDecimalLabelSecondary);
    centralVLayout    ->addLayout(decimalLabelLayout);

    QHBoxLayout*    setValueLayout                = new QHBoxLayout;
    QPushButton*    setLatitudeFromDecimalButton  = new QPushButton("Set Latitude");
    QDoubleSpinBox* latitudeSpinBox               = new QDoubleSpinBox;
    QPushButton*    setLongitudeFromDecimalButton = new QPushButton("Set Longitude");
    QDoubleSpinBox* longitudeSpinBox              = new QDoubleSpinBox;

    latitudeSpinBox->setRange(-90.0, 90.0);
    latitudeSpinBox->setDecimals(8);

    longitudeSpinBox->setRange(-180.0, 180.0);
    longitudeSpinBox->setDecimals(8);

    setValueLayout->addWidget(setLatitudeFromDecimalButton);
    setValueLayout->addWidget(latitudeSpinBox);
    setValueLayout->addWidget(setLongitudeFromDecimalButton);
    setValueLayout->addWidget(longitudeSpinBox);
    centralVLayout->addLayout(setValueLayout);

    connect(setLatitudeFromDecimalButton, &QPushButton::clicked, this, [this, latitudeLineEdit, latitudeSpinBox](){
        latitudeLineEdit->setTextFromDecimalValue(latitudeSpinBox->value());
    }, Qt::DirectConnection);

    connect(setLongitudeFromDecimalButton, &QPushButton::clicked, this, [this, longitudeLineEdit, longitudeSpinBox](){
        longitudeLineEdit->setTextFromDecimalValue(longitudeSpinBox->value());
    }, Qt::DirectConnection);

    connect(latitudeLineEdit, &QLineEdit::textChanged, this, [this, latitudeLineEdit, latitudeDecimalLabel, latitudeLineEditError](){
        latitudeDecimalLabel->setText(QString::number(latitudeLineEdit->textToDecimalValue(), 'f', 10));
        latitudeLineEditError->setTextFromDecimalValue(latitudeLineEdit->textToDecimalValue());
    }, Qt::DirectConnection);

    connect(latitudeLineEditError, &QLineEdit::textChanged, this, [this, latitudeLineEditError, latitudeDecimalLabelSecondary](){
        latitudeDecimalLabelSecondary->setText(QString::number(latitudeLineEditError->textToDecimalValue(), 'f', 10));
    }, Qt::DirectConnection);

    connect(longitudeLineEditError, &QLineEdit::textChanged, this, [this, longitudeLineEditError, longitudeDecimalLabelSecondary](){
        longitudeDecimalLabelSecondary->setText(QString::number(longitudeLineEditError->textToDecimalValue(), 'f', 10));
    }, Qt::DirectConnection);

    connect(longitudeLineEdit, &QLineEdit::textChanged, this, [this, longitudeLineEdit, longitudeDecimalLabel, longitudeLineEditError](){
        longitudeDecimalLabel->setText(QString::number(longitudeLineEdit->textToDecimalValue(), 'f', 10));
        longitudeLineEditError->setTextFromDecimalValue(longitudeLineEdit->textToDecimalValue());
    }, Qt::DirectConnection);

}

MainWindow::~MainWindow(){

    /* NOP */

}
