#include "stellarsolverctrl.h"
#include <QDebug>
#include <QImage>
#include <QApplication>

#define SEP_TBYTE 11

StellarSolverCtrl::StellarSolverCtrl(QObject *parent) : QObject(parent)
{
    StellarSolver temp(SOLVE, stats, m_ImageBuffer, this);
    m_OptionsList = temp.getBuiltInProfiles();
    m_OptionsProfileList.clear();
    for(int i = 0;i < m_OptionsList.size();i++){
        m_OptionsProfileList.push_back((SSolver::Parameters::ParametersProfile)i);
    }
    m_CurrentProfile = SSolver::Parameters::ParametersProfile::PARALLEL_LARGESCALE;
}

bool StellarSolverCtrl::setImage(QImage img)
{
    QImage rgb32 = img.convertToFormat(QImage::Format_RGB32);

    int fitsBitPix =
        8; //Note: This will need to be changed.  I think QT only loads 8 bpp images.  Also the depth method gives the total bits per pixel in the image not just the bits per pixel in each channel.
    switch (fitsBitPix)
    {
        case BYTE_IMG:
            stats.dataType      = SEP_TBYTE;
            stats.bytesPerPixel = sizeof(uint8_t);
            break;
        case SHORT_IMG:
            // Read SHORT image as USHORT
            stats.dataType      = TUSHORT;
            stats.bytesPerPixel = sizeof(int16_t);
            break;
        case USHORT_IMG:
            stats.dataType      = TUSHORT;
            stats.bytesPerPixel = sizeof(uint16_t);
            break;
        case LONG_IMG:
            // Read LONG image as ULONG
            stats.dataType      = TULONG;
            stats.bytesPerPixel = sizeof(int32_t);
            break;
        case ULONG_IMG:
            stats.dataType      = TULONG;
            stats.bytesPerPixel = sizeof(uint32_t);
            break;
        case FLOAT_IMG:
            stats.dataType      = TFLOAT;
            stats.bytesPerPixel = sizeof(float);
            break;
        case LONGLONG_IMG:
            stats.dataType      = TLONGLONG;
            stats.bytesPerPixel = sizeof(int64_t);
            break;
        case DOUBLE_IMG:
            stats.dataType      = TDOUBLE;
            stats.bytesPerPixel = sizeof(double);
            break;
        default:
            logOutput(QString("Bit depth %1 is not supported.").arg(fitsBitPix));
            return false;
    }

    stats.width = static_cast<uint16_t>(rgb32.width());
    stats.height = static_cast<uint16_t>(rgb32.height());
    stats.channels = 3;
    stats.samples_per_channel = stats.width * stats.height;
    clearImageBuffers();
    m_ImageBufferSize = stats.samples_per_channel * stats.channels * static_cast<uint16_t>(stats.bytesPerPixel);
    m_ImageBuffer = new uint8_t[m_ImageBufferSize];
    if (m_ImageBuffer == nullptr)
    {
        logOutput(QString("FITSData: Not enough memory for image_buffer channel. Requested: %1 bytes ").arg(m_ImageBufferSize));
        clearImageBuffers();
        return false;
    }

    auto debayered_buffer = reinterpret_cast<uint8_t *>(m_ImageBuffer);
    auto * original_bayered_buffer = reinterpret_cast<uint8_t *>(rgb32.bits());

    // Data in RGB32, with bytes in the order of B,G,R,A, we need to copy them into 3 layers for FITS

    uint8_t * rBuff = debayered_buffer;
    uint8_t * gBuff = debayered_buffer + (stats.width * stats.height);
    uint8_t * bBuff = debayered_buffer + (stats.width * stats.height * 2);

    int imax = stats.samples_per_channel * 4 - 4;
    for (int i = 0; i <= imax; i += 4)
    {
        *rBuff++ = original_bayered_buffer[i + 2];
        *gBuff++ = original_bayered_buffer[i + 1];
        *bBuff++ = original_bayered_buffer[i + 0];
    }
    resetStellarSolver();

    return true;
}

void StellarSolverCtrl::resetStellarSolver()
{
    if(m_pStellarSolver != nullptr)
    {
        auto *solver = m_pStellarSolver.release();
        solver->disconnect(this);
        if(solver->isRunning())
        {
            connect(solver, &StellarSolver::finished, solver, &StellarSolver::deleteLater);
            solver->abort();
        }
        else
            solver->deleteLater();
    }

    m_pStellarSolver.reset(new StellarSolver(stats, m_ImageBuffer, this));
    connect(m_pStellarSolver.get(), &StellarSolver::logOutput, this, &StellarSolverCtrl::logOutput);
}

void StellarSolverCtrl::setupStellarSolverParameters()
{
    //Index Folder Paths
    QStringList indexFolderPaths;
    indexFolderPaths << "/usr/share/astrometry/";
    m_pStellarSolver->setIndexFolderPaths(indexFolderPaths);

    //These setup Logging if desired
    m_pStellarSolver->setProperty("LogToFile", false);
    QString filename = "";
    if(filename != "" && QFileInfo(filename).dir().exists() && !QFileInfo(filename).isDir()) m_pStellarSolver->m_LogFileName=filename;
    m_pStellarSolver->setLogLevel(LOG_NONE);
    m_pStellarSolver->setSSLogLevel(LOG_NORMAL);
}


//This method runs when the user clicks the Sextract and Solve buttton
void StellarSolverCtrl::startSolve()
{
    prepareForProcesses();
    //This makes sure the solver is done before starting another time
    //That way the timer is accurate.
    while(m_pStellarSolver->isRunning())
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    //Since this tester uses it many times, it doesn't *need* to replace the stellarsolver every time
    resetStellarSolver();
    m_pStellarSolver->setProperty("ProcessType", SOLVE);
    m_pStellarSolver->setProperty("ExtractorType", EXTRACTOR_INTERNAL);
    m_pStellarSolver->setProperty("SolverType", SOLVER_STELLARSOLVER);

    //These set the StellarSolver Parameters
    m_pStellarSolver->setParameters(m_OptionsList.at(m_CurrentProfile));

//  m_pStellarSolver->setParameters(getSettings());

    setupStellarSolverParameters();
    m_pStellarSolver->clearSubFrame();

    //Setting the initial search scale settings
    if(mIsScaleEnabled){
        m_pStellarSolver->setSearchScale(mScaleMin, mScaleMax, DEG_WIDTH);
    }
    if(mIsSearchPositionEnabled){
        m_pStellarSolver->setSearchPositionInDegrees(mSearchPositionRa, mSearchPositionDec);//Ra/Dec:Degree
    }

    connect(m_pStellarSolver.get(), &StellarSolver::ready, this, &StellarSolverCtrl::solverComplete);

    processTimer.start();
    m_pStellarSolver->start();
}

void StellarSolverCtrl::logOutput(QString text)
{
    emit onLogOutput(text);
}

bool StellarSolverCtrl::solverComplete()
{
    disconnect(m_pStellarSolver.get(), &StellarSolver::ready, this, &StellarSolverCtrl::solverComplete);
    qDebug() << QString::asprintf("Elapsed time = %lf", processTimer.elapsed() / 1000.0);
    FITSImage::Solution solution = m_pStellarSolver->getSolution();
    emit onSolverComplete(!m_pStellarSolver->failed() && m_pStellarSolver->solvingDone(), solution);
    return true;
}

SSolver::Parameters StellarSolverCtrl::getSettings()
{
    SSolver::Parameters params;
    params.listName = "Custom";
    params.description = "";
    //These are to pass the parameters to the internal sextractor
    params.apertureShape = SHAPE_CIRCLE;
    params.kron_fact = 2.5;
    params.subpix = 5;
    params.r_min = 3.5;
    //params.inflags
    params.magzero = 20;
    params.minarea = 10;
    params.deblend_thresh = 32;
    params.deblend_contrast = 0.005;
    params.clean = true;
    params.clean_param = 1;
    StellarSolver::createConvFilterFromFWHM(&params, 4);
    params.partition = true;

    //Star Filter Settings
    params.resort = true;
    params.maxSize = 0;
    params.minSize = 0;
    params.maxEllipse = 1.5;
    params.initialKeep = 500;
    params.keepNum = 50;
    params.removeBrightest = 0;
    params.removeDimmest = 0;
    params.saturationLimit = 0;

    //Settings that usually get set by the config file

    params.maxwidth = 180;
    params.minwidth = 1;
    params.inParallel = 1;
    params.multiAlgorithm = MULTI_AUTO;
    params.solverTimeLimit = 600;
    params.resort = 1;
    params.autoDownsample = 1;
    params.downsample = 1;
    params.search_radius = 15;

    //Setting the settings to know when to stop or keep searching for solutions
    params.logratio_tokeep = 20.7233;
    params.logratio_totune = 13.8155;
    params.logratio_tosolve = 20.7233;

    return params;
}


bool StellarSolverCtrl::prepareForProcesses()
{
    if(m_pStellarSolver != nullptr)
    {
        if(m_pStellarSolver->isRunning())
        {
            const SSolver::ProcessType type = static_cast<SSolver::ProcessType>(m_pStellarSolver->property("ProcessType").toInt());
            if(type == SOLVE && !m_pStellarSolver->solvingDone())
            m_pStellarSolver->abort();
        }
    }

    return true;
}

void StellarSolverCtrl::clearImageBuffers()
{
    delete[] m_ImageBuffer;
    m_ImageBuffer = nullptr;
    //m_BayerBuffer = nullptr;
}
