#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGuiApplication>
#include "ui_mainwindow.h"
#include "getRecArea.h"
#include <QScreen>
#include <QObject>
#include <iostream>
#include <QFileDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QScreen* screen{};

    int height;
    int width;
    bool custom;
    bool micRec;
    std::string filename;
    QPoint origin;
    QPoint end;



private slots:
    void on_pushButton_clicked();

    void on_checkBox_2_stateChanged(int arg1);

    void on_checkBox_3_stateChanged(int arg1);

    void on_checkBox_stateChanged(int arg1);

    void on_pushButton_2_clicked();

    void on_pushButton_6_clicked();

public slots:
    void getArea(QPoint origin, QPoint end);

private:
    Ui::MainWindow *ui;
    bool hasEnding (std::string const &fullString, std::string const &ending);

};
#endif // MAINWINDOW_H
