#ifndef STELLARSOLVERCTRL_H
#define STELLARSOLVERCTRL_H

#include <QObject>
#include <stellarsolver.h>
#include <memory>
#include <QElapsedTimer>

class StellarSolverCtrl : public QObject
{
    Q_OBJECT
public:
    explicit StellarSolverCtrl(QObject *parent = nullptr);

    bool setImage(QImage img);
    void startSolve();
    void stopSolve(){
        if(m_pStellarSolver != nullptr)
        {
            if(m_pStellarSolver->isRunning())
            {
                const SSolver::ProcessType type = static_cast<SSolver::ProcessType>(m_pStellarSolver->property("ProcessType").toInt());
                if(type == SOLVE && !m_pStellarSolver->solvingDone())
                m_pStellarSolver->abort();
            }
        }
    }


    QList<SSolver::Parameters::ParametersProfile> getparameterProfileList(){return m_OptionsProfileList;}
    void setParameterProfile(SSolver::Parameters::ParametersProfile profile){m_CurrentProfile = profile;}
    void setScale(bool isEnable, float minScale, float maxScale){
        mIsScaleEnabled = isEnable;
        if(isEnable){
            mScaleMax = maxScale;
            mScaleMin = minScale;
        }
    }
    void setSearchPosition(bool isEnable, float ra, float dec){
        mIsSearchPositionEnabled = isEnable;
        if(isEnable){
            mSearchPositionRa = ra;
            mSearchPositionDec = dec;
        }
    }
    bool isSolvingActive(){
        return m_pStellarSolver != nullptr && !m_pStellarSolver->solvingDone();
    }
signals:
    void onLogOutput(QString text);
    bool onSolverComplete(bool isSuccess, FITSImage::Solution solution);

private slots:
    void logOutput(QString text);
    bool solverComplete();

private:

    std::unique_ptr<StellarSolver> m_pStellarSolver;

    void resetStellarSolver();
    void clearImageBuffers();
    bool prepareForProcesses();
    SSolver::Parameters getSettings();
    void setupStellarSolverParameters();
    float mScaleMin;
    float mScaleMax;
    bool mIsScaleEnabled{false};
    float mSearchPositionRa;
    float mSearchPositionDec;
    bool mIsSearchPositionEnabled{false};

    QList<SSolver::Parameters> m_OptionsList;
    QList<SSolver::Parameters::ParametersProfile> m_OptionsProfileList;
    SSolver::Parameters::ParametersProfile m_CurrentProfile;

    /// Generic data image buffer
    uint8_t *m_ImageBuffer { nullptr };
    /// Above buffer size in bytes
    uint32_t m_ImageBufferSize { 0 };
    FITSImage::Statistic stats;


    QElapsedTimer processTimer;
};

#endif // STELLARSOLVERCTRL_H
