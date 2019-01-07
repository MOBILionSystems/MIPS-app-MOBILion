#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include "comms.h"
#include "mipscomms.h"
#include "cmdlineapp.h"
#include "arbwaveformedit.h"
#include "script.h"
#include "help.h"
#include "rfamp.h"
#include "tcpserver.h"
#include "arb.h"
#include "adc.h"
#include "dio.h"
#include "dcbias.h"
#include "rfdriver.h"
#include "timinggenerator.h"
#include "compressor.h"
#include "properties.h"

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
#include <QMenu>

extern QString Version;

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

class DACchannel : public QWidget
{
    Q_OBJECT
public:
    DACchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    QString ProcessCommand(QString cmd);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    int     Channel;
    Comms   *comms;
    float   b,m;
    QString Units;
    QString Format;
private:
    QFrame      *frmDAC;
    QLineEdit   *Vdac;
    QLabel      *labels[2];
private slots:
    void VdacChange(void);
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

class ESI : public QWidget
{
    Q_OBJECT
public:
    ESI(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    QString ProcessCommand(QString cmd);
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

class ControlPanel : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);

public:
    explicit ControlPanel(QWidget *parent, QString CPfileName, QList<Comms*> S, Properties *prop);
    ~ControlPanel();
    virtual void reject();
    void Update(void);
    Q_INVOKABLE QString Save(QString Filename);
    Q_INVOKABLE QString Load(QString Filename);
    void InitMIPSsystems(QString initFilename);
    QList<Comms*> Systems;
    QStatusBar  *statusBar;
    DCBchannel  *FindDCBchannel(QString name);
    bool LoadedConfig;
    Properties *properties;
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
    Q_INVOKABLE QString popupUserInput(QString title, QString message);

private:
    Comms            *FindCommPort(QString name, QList<Comms*> Systems);
    Ui::ControlPanel *ui;
    TCPserver *tcp;
    QMenu *contextMenu2Dplot;
    QAction *GeneralHelp;
    QAction *MIPScommands;
    QAction *ScriptHelp;
    QAction *ThisHelp;
    QString HelpFile;
    QString ControlPanelFile;

    QList<TextLabel *>  TextLabels;
    QList<RFchannel *>  RFchans;
    QList<ADCchannel *> ADCchans;
    QList<DACchannel *> DACchans;
    QList<DCBchannel *> DCBchans;
    QList<DCBoffset *>  DCBoffsets;
    QList<DCBenable *>  DCBenables;
    QList<DIOchannel *> DIOchannels;
    QList<ESI *>        ESIchans;
    QList<ARBchannel *> ARBchans;
    QList<RFamp *>      rfa;
    IFTtiming           *IFT;
    TimingControl       *TC;
    Compressor          *comp;
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
    DCBiasGroups     *DCBgroups;
    QPushButton      *MIPScommsButton;
    QPushButton      *ARBcompressorButton;
    QPushButton      *ScriptButton;
    ScriptingConsole *scriptconsole;
    QUdpSocket       *udpSocket;
    Help             *help;
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
    void popupHelp(QPoint);
    void slotGeneralHelp(void);
    void slotMIPScommands(void);
    void slotScriptHelp(void);
    void slotThisControlPanelHelp(void);
    void tcpCommand(void);
    void pbARBcompressor(void);
};

#endif // CONTROLPANEL_H
