#ifndef CMDLINEAPP_H
#define CMDLINEAPP_H

#include <QDialog>
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
#include <QDebug>

namespace Ui {
class cmdlineapp;
}

class cmdlineapp : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);
    void AppCompleted(void);
    void Ready(void);

public:
    explicit cmdlineapp(QWidget *parent = 0);
    ~cmdlineapp();
    void reject();
    QProcess process;
    QString  appPath;
    QString  ReadyMessage;
    void Execute(void);

private:
    Ui::cmdlineapp *ui;

private slots:
    void readProcessOutput(void);
    void readMessage(void);
    void AppFinished(void);
};

#endif // CMDLINEAPP_H
