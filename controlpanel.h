#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include "comms.h"
#include "mipscomms.h"
#include "cmdlineapp.h"
#include "arbwaveformedit.h"
#include "script.h"

#include <QDialog>
#include <QDebug>
#include <QLineEdit>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QProcess>
#include <QKeyEvent>
#include <QBoxLayout>
#include <QUdpSocket>

//
// To dos:
//
// Need to add classes for:
// DIO

namespace Ui {
class ControlPanel;
}

class ScriptingConsole;

class TextLabel : public QWidget
{
    Q_OBJECT
public:
    TextLabel(QWidget *parent, QString name, int size, int x, int y);
    void Show(void);
    QWidget *p;
    QString Title;
    int     X,Y;
    int     Size;
private:
    QLabel      *label;
};

class Shutdown : public QWidget
{
    Q_OBJECT
signals:
    void ShutdownSystem(void);
    void EnableSystem(void);
public:
    Shutdown(QWidget *parent, QString name, int x, int y);
    void Show(void);
    QWidget *p;
    QString Title;
    int     X,Y;
private:
    QPushButton *pbShutdown;
private slots:
    void pbPressed(void);
};

class SaveLoad : public QWidget
{
    Q_OBJECT
signals:
    void Save(void);
    void Load(void);
public:
    SaveLoad(QWidget *parent, QString nameSave, QString nameLoad, int x, int y);
    void Show(void);
    QWidget *p;
    QString TitleSave;
    QString TitleLoad;
    int     X,Y;
private:
    QPushButton *pbSave;
    QPushButton *pbLoad;
private slots:
    void pbSavePressed(void);
    void pbLoadPressed(void);
};

class RFchannel : public QWidget
{
    Q_OBJECT

public:
    RFchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    void Shutdown(void);
    void Restore(void);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    int     Channel;
    Comms   *comms;
    bool    isShutdown;
private:
    QGroupBox   *gbRF;
    QLineEdit   *Drive;
    QLineEdit   *Freq;
    QLineEdit   *RFP;
    QLineEdit   *RFN;
    QLineEdit   *Power;
    QPushButton *Tune;
    QPushButton *Retune;
    QLabel      *labels[10];
    QString     activeDrive;
    bool Updating;
    bool UpdateOff;
private slots:
    void DriveChange(void);
    void FreqChange(void);
    void TunePressed(void);
    void RetunePressed(void);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class DCBchannel : public QWidget
{
    Q_OBJECT
public:
    DCBchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    QWidget               *p;
    QString               Title;
    int                   X,Y;
    QString               MIPSnm;
    int                   Channel;
    Comms                 *comms;
    QLineEdit             *Vsp;
    bool                  LinkEnable;
    QList<DCBchannel*>    DCBs;
    float                 CurrentVsp;
private:
    QFrame                *frmDCB;
    QLineEdit             *Vrb;
    QLabel                *labels[2];
    bool Updating;
    bool UpdateOff;
private slots:
    void VspChange(void);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class DCBoffset : public QWidget
{
    Q_OBJECT
public:
    DCBoffset(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    int     Channel;
    Comms   *comms;
private:
    QFrame      *frmDCBO;
    QLineEdit   *Voff;
    QLabel      *labels[2];
private slots:
    void VoffChange(void);
};

class DCBenable : public QWidget
{
    Q_OBJECT
public:
    DCBenable(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    void Shutdown(void);
    void Restore(void);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    Comms   *comms;
    bool    isShutdown;
private:
    QFrame      *frmDCBena;
    QCheckBox   *DCBena;
    bool        activeEnableState;
private slots:
    void DCBenaChange(void);
};

class DCbiasGroupsEventFilter : public QObject
{
     Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class DCBiasGroups : public QWidget
{
    Q_OBJECT
signals:
    void disable(void);
    void enable(void);
public:
    DCBiasGroups(QWidget *parent, int x, int y);
    void Show(void);
    QWidget *p;
    int     X,Y;
    QComboBox   *comboGroups;
    bool SetValues(QString strVals);
    QString Report(void);
private:
    QGroupBox   *gbDCBgroups;
    QCheckBox   *DCBenaGroups;
private slots:
    void slotEnableChange(void);
};

class DIOchannel : public QWidget
{
    Q_OBJECT
public:
    DIOchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    QString Channel;
    Comms   *comms;
private:
    QFrame      *frmDIO;
    QCheckBox   *DIO;
private slots:
    void DIOChange(void);
};

class ESI : public QWidget
{
    Q_OBJECT
public:
    ESI(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    void Shutdown(void);
    void Restore(void);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    int     Channel;
    Comms   *comms;
    bool    isShutdown;
private:
    QFrame      *frmESI;
    QLineEdit   *ESIsp;
    QLineEdit   *ESIrb;
    QCheckBox   *ESIena;
    QLabel      *labels[2];
    bool        activeEnableState;
private slots:
    void ESIChange(void);
    void ESIenaChange(void);
};

class ARBchannel : public QWidget
{
    Q_OBJECT
public:
    ARBchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    int     Channel;
    Comms   *comms;
    QStatusBar  *statusBar;
private:
    QGroupBox    *gbARB;
    QLineEdit    *leSWFREQ;
    QLineEdit    *leSWFVRNG;
    QLineEdit    *leSWFVAUX;
    QLineEdit    *leSWFVOFF;
    QRadioButton *SWFDIR_FWD;
    QRadioButton *SWFDIR_REV;
    QComboBox    *Waveform;
    QPushButton  *EditWaveform;
    QLabel      *labels[10];
    ARBwaveformEdit *ARBwfEdit;
private slots:
    void leChange(void);
    void rbChange(void);
    void wfChange(void);
    void wfEdit(void);
    void ReadWaveform(void);
};

class IFTtiming : public QWidget
{
    Q_OBJECT
signals:
    void dataAcquired(QString filePath);
public:
    IFTtiming(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    void AcquireData(QString path);
    void Dismiss(void);
    QString MakePathUnique(QString path);
    QWidget *p;
    QString Title;
    QString Acquire;
    int     X,Y;
    QString MIPSnm;
    QString filePath;
    Comms   *comms;
    QString Enable;
    QStatusBar  *statusBar;
    DCBchannel  *Grid1;
    DCBchannel  *Grid2;
    DCBchannel  *Grid3;
    QProcess    process;
    bool        TableDownloaded;
    bool        Acquiring;

private:
    QGroupBox    *gbIFT;
    QLineEdit    *leFillTime;
    QLineEdit    *leTrapTime;
    QLineEdit    *leReleaseTime;
    QLineEdit    *leGrid1FillV;
    QLineEdit    *leGrid2ReleaseV;
    QLineEdit    *leGrid3ReleaseV;
    QLineEdit    *Accumulations;
    QLineEdit    *FrameSize;
    QLineEdit    *Table;
    QComboBox    *ClockSource;
    QComboBox    *TriggerSource;
    QPushButton  *GenerateTable;
    QPushButton  *Download;
    QPushButton  *Trigger;
    QPushButton  *Abort;
    QLabel       *labels[11];
    cmdlineapp   *cla;
public slots:
    void pbGenerate(void);
    void pbDownload(void);
    void pbTrigger(void);
    void pbAbort(void);
    void AppReady(void);
    void slotAppFinished(void);
    void tblObsolite(void);
};

class ControlPanel : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);

public:
    explicit ControlPanel(QWidget *parent, QList<Comms*> S);
    ~ControlPanel();
    virtual void reject();
    void Update(void);
    Q_INVOKABLE QString Save(QString Filename);
    Q_INVOKABLE QString Load(QString Filename);
    void InitMIPSsystems(QString initFilename);
    QList<Comms*> Systems;
    QStatusBar  *statusBar;
    DCBchannel  *FindDCBchannel(QString name);
    // The following functions are for the scripting system
    Q_INVOKABLE bool SendCommand(QString MIPSname, QString message);
    Q_INVOKABLE QString SendMess(QString MIPSname, QString message);
    Q_INVOKABLE void SystemEnable(void);
    Q_INVOKABLE void SystemShutdown(void);
    Q_INVOKABLE void Acquire(QString filePath);
    Q_INVOKABLE bool isAcquiring(void);
    Q_INVOKABLE void DismissAcquire(void);
    Q_INVOKABLE void msDelay(int ms);
    Q_INVOKABLE void statusMessage(QString message);
    Q_INVOKABLE void popupMessage(QString message);
    Q_INVOKABLE bool popupYesNoMessage(QString message);

private:
    Comms            *FindCommPort(QString name, QList<Comms*> Systems);
    Ui::ControlPanel *ui;

    int         numTextLabels;
    TextLabel   **TextLabels;
    int         numRFchannels;
    RFchannel   **RFchans;
    int         numDCBchannels;
    DCBchannel  **DCBchans;
    int         numDCBoffsets;
    DCBoffset   **DCBoffsets;
    int         numDCBenables;
    DCBenable   **DCBenables;
    int         numDIOchannels;
    DIOchannel  **DIOchannels;
    int         numESIchannels;
    ESI         **ESIchans;
    int         numARBchannels;
    ARBchannel  **ARBchans;
    IFTtiming   *IFT;
    int         UpdateHoldOff;
    bool        UpdateStop;
    bool        ShutdownFlag;
    bool        RestoreFlag;
    bool        SystemIsShutdown;
    bool        StartMIPScomms;
    Script      *script;
    Shutdown    *SD;
    SaveLoad    *SL;
    MIPScomms   *mc;
    DCBiasGroups *DCBgroups;
    QPushButton *MIPScommsButton;
    QPushButton *ScriptButton;
    ScriptingConsole *scriptconsole;
    QUdpSocket *udpSocket;
public slots:
    void pbSD(void);
    void pbSE(void);
    void pbSave(void);
    void pbLoad(void);
    void pbMIPScomms(void);
    void CloseMIPScomms(void);
    void scriptShutdown(void);
    void scriptLoad(QString Filename);
    void DCBgroupDisable(void);
    void DCBgroupEnable(void);
    void slotDataAcquired(QString filepath);
    void pbScript(void);
};

#endif // CONTROLPANEL_H
