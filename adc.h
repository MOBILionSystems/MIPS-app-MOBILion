#ifndef ADC_H
#define ADC_H

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

class ADC: public QDialog
{
    Q_OBJECT

public:
    ADC(Ui::MIPS *w, Comms *c);
    void Update(bool UpdateSelected = false);
//    void Save(QString Filename);
//    void Load(QString Filename);

    Ui::MIPS *ui;
    Comms *comms;
    // ADC buffer and parameters
    quint16 *ADCbuffer;
    int *ADCbufferSum;
    int NumScans;

private slots:
//    void UpdateADC(void);
    void ADCupdated(void);
    void ADCsetup(void);
    void ADCtrigger(void);
    void ADCabort(void);
    void ADCrecordingDone(void);
    void ADCvectorReady(void);
    void SetZoom(void);
};

#endif // ADC_H
