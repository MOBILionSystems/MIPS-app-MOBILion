#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <QDialog>
#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QApplication>
#include <QFileInfo>
#include <QKeyEvent>
#include <QCheckBox>
#include <QComboBox>

#include "comms.h"

namespace Ui {
class Compressor;
}

class Compressor : public QDialog
{
    Q_OBJECT

public:
    explicit Compressor(QWidget *parent, QString name, QString MIPSname);
    ~Compressor();
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    QString ProcessCommand(QString cmd);

    Comms   *comms;
    QString Title;
    QString MIPSnm;

private:
    Ui::Compressor *ui;
    bool Updating;
    bool UpdateOff;

private slots:
    void Updated(void);
    void pbARBforceTriggerSlot(void);
};

#endif // COMPRESSOR_H
