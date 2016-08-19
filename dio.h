#ifndef DIO_H
#define DIO_H

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

class DIO: public QDialog
{
    Q_OBJECT

public:
    DIO(Ui::MIPS *w, Comms *c);
    void Update(void);
    void Save(QString Filename);
    void Load(QString Filename);

    Ui::MIPS *dui;
    Comms *comms;

private slots:
    void UpdateDIO(void);
    void DOUpdated(void);
    void TrigHigh(void);
    void TrigLow(void);
    void TrigPulse(void);
};

#endif // DIO_H
