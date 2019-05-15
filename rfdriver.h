#ifndef RFDRIVER_H
#define RFDRIVER_H

#include "ui_mips.h"
#include "mips.h"
#include "comms.h"

#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QApplication>
#include <QFileInfo>

class RFdriver : public QDialog
{
     Q_OBJECT

public:
    RFdriver(Ui::MIPS *w, Comms *c);
    void SetNumberOfChannels(int num);
    void Update(void);
    void Save(QString Filename);
    void Load(QString Filename);

    Ui::MIPS *rui;
    Comms *comms;
    int NumChannels;

private slots:
    void UpdateRFdriver(void);
    void leSRFFRQ_editingFinished();
    void leSRFDRV_editingFinished();
    void AutoTune(void);
    void AutoRetune(void);
};

class RFchannel : public QWidget
{
    Q_OBJECT

public:
    RFchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void    Show(void);
    void    Update(QString sVals = "");
    QString Report(void);
    bool    SetValues(QString strVals);
    void    Shutdown(void);
    void    Restore(void);
    QString ProcessCommand(QString cmd);
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
    bool        Updating;
    bool        UpdateOff;
private slots:
    void DriveChange(void);
    void FreqChange(void);
    void TunePressed(void);
    void RetunePressed(void);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class RFCchannel : public QWidget
{
    Q_OBJECT

public:
    RFCchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void    Show(void);
    void    Update(QString sVals = "");
    QString Report(void);
    bool    SetValues(QString strVals);
    void    Shutdown(void);
    void    Restore(void);
    QString ProcessCommand(QString cmd);
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
    QLineEdit   *Setpoint;
    QLineEdit   *Freq;
    QLineEdit   *RFP;
    QLineEdit   *RFN;
    QLineEdit   *Power;
    QRadioButton *Open_Loop;
    QRadioButton *Closed_Loop;
    QPushButton *Tune;
    QPushButton *Retune;
    QLabel      *labels[12];
    QString     activeDrive;
    QString     activeSetpoint;
    bool        Updating;
    bool        UpdateOff;
private slots:
    void DriveChange(void);
    void SetpointChange(void);
    void FreqChange(void);
    void TunePressed(void);
    void RetunePressed(void);
    void rbChange(void);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // RFDRIVER_H
