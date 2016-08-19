#ifndef PROGRAM_H
#define PROGRAM_H

#include "ui_mips.h"
#include "mips.h"
#include "comms.h"
#include "console.h"

#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QApplication>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <QFileDialog>


class Program : public QDialog
{
     Q_OBJECT

public:
    Program(Ui::MIPS *w, Comms *c, Console *con);

    Ui::MIPS *pui;
    Comms *comms;
    Console *console;
    QProcess process;
    QString  appPath;

private slots:
    void programMIPS(void);
    void executeProgrammerCommand(QString cmd);
    void setBootloaderBootBit(void);
    void saveMIPSfirmware(void);
    void readProcessOutput(void);
};

#endif // PROGRAM_H
