#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>

class MainWindow : public QMainWindow{

    Q_OBJECT

public:

    MainWindow(QWidget* parent = nullptr);

    void setupDMSWidget();

    void setupDoubleWidget();

    ~MainWindow();

    QTabWidget* m_tabWidget;
    QWidget*    m_dmsWidget;
    QWidget*    m_doubleWidget;

};

#endif // MAINWINDOW_H
