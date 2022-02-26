#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QImageReader>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&mStellarSolver, &StellarSolverCtrl::onLogOutput, this, &MainWindow::logOutput);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//This runs when the solver is complete.  It reports the time taken, prints a message, and adds the solution to the results table.
bool MainWindow::solverComplete(bool isSuccess, FITSImage::Solution solution)
{

    disconnect(&mStellarSolver, &StellarSolverCtrl::onSolverComplete, this, &MainWindow::solverComplete);

    if(isSuccess){
        logOutput("RA (J2000)" + StellarSolver::raString(solution.ra));
        logOutput("DEC (J2000)"+ StellarSolver::decString(solution.dec));
        logOutput("Orientation˚"+ QString::number(solution.orientation));
        logOutput("Field Width \'" + QString::number(solution.fieldWidth));
        logOutput("Field Height \'"+ QString::number(solution.fieldHeight));
        logOutput("PixScale \"" + QString::number(solution.pixscale));
        logOutput("Parity" + solution.parity);
    }
    return isSuccess;
}

//These methods are for the logging of information to the textfield at the bottom of the window.
void MainWindow::logOutput(QString text)
{
    qDebug() << text;
}


void MainWindow::on_pushButtonSolve_clicked()
{
    connect(&mStellarSolver, &StellarSolverCtrl::onSolverComplete, this, &MainWindow::solverComplete);

    mStellarSolver.setImage(mImg);
    mStellarSolver.setIndexPath("/usr/share/astrometry/");
    mStellarSolver.setParameterProfile(SSolver::Parameters::ParametersProfile::PARALLEL_LARGESCALE);
    mStellarSolver.setScale(true, 20, 40);
    mStellarSolver.setSearchPosition(true, 65, 0);
    mStellarSolver.startSolve();
}

bool MainWindow::imageLoad()
{
    QString dirPath = "";
    QString fileURL = QFileDialog::getOpenFileName(nullptr, "Load Image", dirPath,
                      "Images (*.bmp *.gif *.jpg *.jpeg *.tif *.tiff)");
    if (fileURL.isEmpty())
        return false;
    QFileInfo fileInfo(fileURL);
    if(!fileInfo.exists())
        return false;

    dirPath = fileInfo.absolutePath();

    QImageReader fileReader(fileURL.toLocal8Bit());

    if (QImageReader::supportedImageFormats().contains(fileReader.format()) == false)
    {
        logOutput("Failed to convert" + fileURL + "to FITS since format, " + fileReader.format() +
                  ", is not supported in Qt");
        return false;
    }

    if(!mImg.load(fileURL.toLocal8Bit()))
    {
        logOutput("Failed to open image.");
        return false;
    }
    return true;
}


void MainWindow::on_pushButtonLoadImage_clicked()
{
    imageLoad();
}

