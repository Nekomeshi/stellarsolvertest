#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "stellarsolverctrl.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    bool imageLoad();
    bool loadOtherFormat();

    QImage mImg;
    StellarSolverCtrl mStellarSolver{this};

public slots:
    void logOutput(QString text);
    bool solverComplete(bool isSuccess, FITSImage::Solution solution);

private slots:
    void on_pushButtonLoadImage_clicked();
    void on_pushButtonSolve_clicked();
};

#endif // MAINWINDOW_H
