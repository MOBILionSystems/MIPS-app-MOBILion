#ifndef MIPSCOMMS_H
#define MIPSCOMMS_H

#include <QDialog>
#include <QMessageBox>
#include <QtSerialPort/QSerialPort>
#include <QTime>
#include <QThread>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QCursor>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QtNetwork/QTcpSocket>
#include <QInputDialog>

#include "comms.h"

namespace Ui {
class MIPScomms;
}

class MIPScomms : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);

public:
    explicit MIPScomms(QWidget *parent, QList<Comms*> S);
    ~MIPScomms();
    virtual void reject();
    QList<Comms*> Systems;

private:
    Ui::MIPScomms *ui;
private slots:
    void CommandEntered(void);
    void readData2Console(void);
};

#endif // MIPSCOMMS_H
