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

    void setupPhoneWidget();

    ~MainWindow();

    QTabWidget* m_tabWidget;
    QWidget*    m_dmsWidget;
    QWidget*    m_doubleWidget;
    QWidget*    m_phoneWidget;

};

#endif // MAINWINDOW_H
