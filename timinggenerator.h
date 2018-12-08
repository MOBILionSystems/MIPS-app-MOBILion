#ifndef TIMINGGENERATOR_H
#define TIMINGGENERATOR_H

#include "comms.h"
#include "cmdlineapp.h"
#include "dcbias.h"
#include "cdirselectiondlg.h"

#include <QDialog>
#include <QStatusBar>
#include <QProcess>
#include <QGroupBox>
#include <QInputDialog>

bool DownloadTable(Comms *comms, QString Table, QString ClockSource, QString TriggerSource);
QString MakePathUnique(QString path);

typedef struct
{
    QString Name;
    QString Channel;
    int     Start;
    int     Width;
    float   Value;
    float   ValueOff;
} Event;

class AcquireData : public QWidget
{
    Q_OBJECT
signals:
    void dataAcquired(QString filePath);
public:
    AcquireData(QWidget *parent = 0);
    void        StartAcquire(QString path, int FrameSize, int Accumulations);
    void        Dismiss(void);
    Comms       *comms;
    QStatusBar  *statusBar;
    QString     filePath;
    QProcess    process;
    bool        TableDownloaded;
    bool        Acquiring;
    QString     Acquire;

private:
    cmdlineapp   *cla;

private slots:
    void slotAppReady(void);
    void slotAppFinished(void);
};

namespace Ui {
class TimingGenerator;
}

class AcquireData;

class TimingGenerator : public QDialog
{
    Q_OBJECT

public:
    explicit TimingGenerator(QWidget *parent, QString name, QString MIPSname);
    ~TimingGenerator();
    void    AddSignal(QString Title, QString Chan);
    QString Report(void);
    bool    SetValues(QString strVals);
    QString ProcessCommand(QString cmd);
    QWidget     *p;
    QString     Title;
    QString     MIPSnm;
    Comms       *comms;
    QStatusBar  *statusBar;
    Event       *selectedEvent;
    QList<Event *>  Events;
    Ui::TimingGenerator *ui;

public slots:
    void slotEventChange(int index);
    void slotEventUpdated(void);
    void slotGenerate(void);
};

class TimingControl : public QWidget
{
    Q_OBJECT
signals:
    void dataAcquired(QString filePath);
public:
    TimingControl(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void              Show(void);
    void              AcquireData(QString path);
    class AcquireData *AD;
    QWidget           *p;
    QString           Title;
    QString           Acquire;
    int               X,Y;
    QString           MIPSnm;
    QString           filePath;
    Comms             *comms;
    QStatusBar        *statusBar;
    QProcess          process;
    TimingGenerator   *TG;
    bool              TableDownloaded;
    bool              Acquiring;

private:
    QGroupBox        *gbTC;
    QPushButton      *Edit;
    QPushButton      *Trigger;
    QPushButton      *Abort;
    cmdlineapp       *cla;

public slots:
    void pbEdit(void);
    void pbTrigger(void);
    void pbAbort(void);
    void slotDataAcquired(QString);
};

class IFTtiming : public QWidget
{
    Q_OBJECT
signals:
    void dataAcquired(QString filePath);
public:
    IFTtiming(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void        Show(void);
    QString     Report(void);
    bool        SetValues(QString strVals);
    void        AcquireData(QString path);
    QWidget     *p;
    class AcquireData *AD;
    QString     Title;
    int         X,Y;
    QString     MIPSnm;
    QString     filePath;
    Comms       *comms;
    QString     Enable;
    QStatusBar  *statusBar;
    DCBchannel  *Grid1;
    DCBchannel  *Grid2;
    DCBchannel  *Grid3;
    QProcess    process;
    bool        TableDownloaded;
    //bool        Acquiring;

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
public slots:
    void slotDataAcquired(QString);
    void pbGenerate(void);
    void pbDownload(void);
    void pbTrigger(void);
    void pbAbort(void);
    void tblObsolite(void);
};

#endif // TIMINGGENERATOR_H
