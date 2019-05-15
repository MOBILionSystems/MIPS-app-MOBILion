#ifndef RFAMP_H
#define RFAMP_H

#include <QDialog>
#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QApplication>
#include <QFileInfo>
#include <QKeyEvent>


#include "comms.h"

namespace Ui {
class RFamp;
}

class RFamp : public QDialog
{
    Q_OBJECT

public:
    explicit RFamp(QWidget *parent, QString name, QString MIPSname, int Module);
    ~RFamp();
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    void Shutdown(void);
    void Restore(void);

    int     Channel;
    Comms   *comms;
    QString Title;
    QString MIPSnm;
    bool    isShutdown;
    bool    activeEnableState;

private:
    Ui::RFamp *ui;
    bool Updating;
    bool UpdateOff;
    QWidget *p;

private slots:
    void Updated(void);
    void slotUpdate(void);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // RFAMP_H
