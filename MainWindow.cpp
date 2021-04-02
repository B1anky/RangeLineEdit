#include "MainWindow.h"
#include "LatitudeLineEdit.h"
#include "LongitudeLineEdit.h"
#include "DoubleLineEdit.h"
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{

    m_tabWidget = new QTabWidget;
    setCentralWidget(m_tabWidget);

    setupDMSWidget();
    setupDoubleWidget();

}

void MainWindow::setupDMSWidget(){

    m_dmsWidget = new QWidget;

    QVBoxLayout* centralVLayout = new QVBoxLayout;
    m_dmsWidget->setLayout(centralVLayout);

    QHBoxLayout*       lineEditLayout    = new QHBoxLayout;
    LatitudeLineEdit*  latitudeLineEdit  = new LatitudeLineEdit(nullptr, 2);
    LongitudeLineEdit* longitudeLineEdit = new LongitudeLineEdit(nullptr, 2);
    lineEditLayout->addWidget(latitudeLineEdit);
    lineEditLayout->addWidget(longitudeLineEdit);
    centralVLayout->addLayout(lineEditLayout);

    QHBoxLayout*       lineEditLayoutError    = new QHBoxLayout;
    LatitudeLineEdit*  latitudeLineEditError  = new LatitudeLineEdit(nullptr, 2);
    LongitudeLineEdit* longitudeLineEditError = new LongitudeLineEdit(nullptr, 2);
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
        latitudeLineEdit->setValue(latitudeSpinBox->value());
    }, Qt::DirectConnection);

    connect(setLongitudeFromDecimalButton, &QPushButton::clicked, this, [this, longitudeLineEdit, longitudeSpinBox](){
        longitudeLineEdit->setValue(longitudeSpinBox->value());
    }, Qt::DirectConnection);

    connect(latitudeLineEdit, &QLineEdit::textChanged, this, [this, latitudeLineEdit, latitudeDecimalLabel, latitudeLineEditError](){
        latitudeDecimalLabel->setText(QString::number(latitudeLineEdit->value(), 'f', 10));
        latitudeLineEditError->setValue(latitudeLineEdit->value());
    }, Qt::DirectConnection);

    connect(latitudeLineEditError, &QLineEdit::textChanged, this, [this, latitudeLineEditError, latitudeDecimalLabelSecondary](){
        latitudeDecimalLabelSecondary->setText(QString::number(latitudeLineEditError->value(), 'f', 10));
    }, Qt::DirectConnection);

    connect(longitudeLineEditError, &QLineEdit::textChanged, this, [this, longitudeLineEditError, longitudeDecimalLabelSecondary](){
        longitudeDecimalLabelSecondary->setText(QString::number(longitudeLineEditError->value(), 'f', 10));
    }, Qt::DirectConnection);

    connect(longitudeLineEdit, &QLineEdit::textChanged, this, [this, longitudeLineEdit, longitudeDecimalLabel, longitudeLineEditError](){
        longitudeDecimalLabel->setText(QString::number(longitudeLineEdit->value(), 'f', 10));
        longitudeLineEditError->setValue(longitudeLineEdit->value());
    }, Qt::DirectConnection);

    m_tabWidget->addTab(m_dmsWidget, "DMS");

}

void MainWindow::setupDoubleWidget(){

    m_doubleWidget = new QWidget;

    QVBoxLayout* centralVLayout = new QVBoxLayout;
    m_doubleWidget->setLayout(centralVLayout);

    QHBoxLayout*    lineEditLayout      = new QHBoxLayout;
    DoubleLineEdit* doubleLineEdit      = new DoubleLineEdit(nullptr, 2);
    DoubleLineEdit* doubleLineEditError = new DoubleLineEdit(nullptr, 2);
    lineEditLayout->addWidget(doubleLineEdit);
    lineEditLayout->addWidget(doubleLineEditError);
    centralVLayout->addLayout(lineEditLayout);

    QHBoxLayout* decimalLabelLayout           = new QHBoxLayout;
    QLabel*      doubleLineEditLabel          = new QLabel;
    QLabel*      doubleLineEditLabelSecondary = new QLabel;

    decimalLabelLayout->addWidget(doubleLineEditLabel);
    decimalLabelLayout->addWidget(doubleLineEditLabelSecondary);
    centralVLayout    ->addLayout(decimalLabelLayout);

    QHBoxLayout*    setValueLayout             = new QHBoxLayout;
    QPushButton*    setDoubleFromDecimalButton = new QPushButton("Set Double");
    QDoubleSpinBox* doubleSpinBox              = new QDoubleSpinBox;

    doubleSpinBox->setRange(-INT_MAX, INT_MAX);
    doubleSpinBox->setDecimals(8);

    setValueLayout->addWidget(setDoubleFromDecimalButton);
    setValueLayout->addWidget(doubleSpinBox);
    centralVLayout->addLayout(setValueLayout);

    connect(setDoubleFromDecimalButton, &QPushButton::clicked, this, [this, doubleLineEdit, doubleSpinBox](){
        doubleLineEdit->setValue(doubleSpinBox->value());
    }, Qt::DirectConnection);

    connect(doubleLineEdit, &QLineEdit::textChanged, this, [this, doubleLineEdit, doubleLineEditLabel, doubleLineEditError](){
        doubleLineEditLabel->setText(QString::number(doubleLineEdit->value(), 'f', 10));
        doubleLineEditError->setValue(doubleLineEdit->value());
    }, Qt::DirectConnection);

    connect(doubleLineEditError, &QLineEdit::textChanged, this, [this, doubleLineEditError, doubleLineEditLabelSecondary](){
        doubleLineEditLabelSecondary->setText(QString::number(doubleLineEditError->value(), 'f', 10));
    }, Qt::DirectConnection);

    m_tabWidget->addTab(m_doubleWidget, "Double");

}

MainWindow::~MainWindow(){

    /* NOP */

}
