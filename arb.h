#ifndef ARB_H
#define ARB_H

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

class Help;

class ARB : public QDialog
{
     Q_OBJECT

public:
    ARB(Ui::MIPS *w, Comms *c);
    void SetNumberOfChannels(int num);
    void Update(void);
    void Save(QString Filename);
    void Load(QString Filename);

private:
    Ui::MIPS *aui;
    Comms *comms;
    int NumChannels;
    QString LogString;
    Help *LogedData;
    bool Compressor;

private slots:
    void ARBUpdated(void);
    void SetARBchannel(void);
    void SetARBchannelRange(void);
    void ARBtrigger(void);
    void SetARBchannel_2(void);
    void SetARBchannelRange_2(void);
    void ARBtrigger_2(void);
    void ARBviewLog(void);
    void ARBclearLog(void);
    void ARBupdate(void);
    void ARBtabSelected(void);
    void ARBtypeSelected(void);
    void rbTW1fwd(void);
    void rbTW1rev(void);
    void ARBtypeSelected2(void);
    void rbTW2fwd(void);
    void rbTW2rev(void);
    // Compressor
    void rbModeCompress(void);
    void rbModeNormal(void);
    void rbSwitchClose(void);
    void rbSwitchOpen(void);
    void pbForceTrigger(void);

};

#endif // ARB_H
