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

#endif // RFDRIVER_H
