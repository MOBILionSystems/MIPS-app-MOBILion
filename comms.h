#ifndef COMMS
#define COMMS

#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QTime>
#include <QApplication>
#include <QtNetwork/QTcpSocket>
#include <QFileInfo>
#include <QFileDialog>

#include "settingsdialog.h"
#include "ringbuffer.h"

class Comms : public QDialog
{
     Q_OBJECT

signals:
    void DataReady(void);

public:
    explicit Comms(SettingsDialog *settings, QString Host, QStatusBar *statusbar);
    bool ConnectToMIPS();
    void DisconnectFromMIPS();
    bool SendCommand(QString name, QString message);
    bool SendCommand(QString message);
    QString SendMessage(QString name, QString message);
    QString SendMessage(QString message);
    void SendString(QString name, QString message);
    void SendString(QString message);
    void writeData(const QByteArray &data);
    bool openSerialPort();
    void closeSerialPort();
    void waitforline(int timeout);
    char getchar(void);
    void GetMIPSfile(QString MIPSfile, QString LocalFile);
    void PutMIPSfile(QString MIPSfile, QString LocalFile);
    void GetEEPROM(QString FileName, QString Board, int Addr);
    void PutEEPROM(QString FileName, QString Board, int Addr);
    bool isConnected(void);
    QString getline(void);
    int CalculateCRC(QByteArray fdata);
    QString MIPSname;
    QByteArray readall(void);
    bool isMIPS(QString port);

    QSerialPort *serial;
    QStatusBar *sb;
    SettingsDialog::Settings p;
    QTcpSocket client;
    bool client_connected;
    QString host;
    RingBuffer rb;

private slots:
    void handleError(QSerialPort::SerialPortError error);
    void readData2RingBuffer(void);
    void connected(void);
    void disconnected(void);
};

#endif // COMMS

