#include "MainWindow.h"
#include "PositionalLineEdits.h"
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    PositionalLineEdits* latitudeLineEdit     = new PositionalLineEdits(PositionalLineEdits::Type::LATITUDE,  2);
    PositionalLineEdits* longitudinalLineEdit = new PositionalLineEdits(PositionalLineEdits::Type::LONGITUDE, 2);
    QWidget* centralWidget = new QWidget;
    QHBoxLayout* lineEditLayout = new QHBoxLayout;
    centralWidget->setLayout(lineEditLayout);
    lineEditLayout->addWidget(latitudeLineEdit);
    lineEditLayout->addWidget(longitudinalLineEdit);
    setCentralWidget(centralWidget);

}

MainWindow::~MainWindow()
{
}

