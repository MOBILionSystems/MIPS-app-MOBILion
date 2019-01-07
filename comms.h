#ifndef COMMS_H
#define COMMS_H

#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QTime>
#include <QTimer>
#include <QApplication>
#include <QtNetwork/QTcpSocket>
#include <QFileInfo>
#include <QFileDialog>

#include "settingsdialog.h"
#include "ringbuffer.h"

enum ADCreadStates
  {
     ADCdone,
     WaitingForHeader,
     ReadingHeader,
     ReadingData,
     ReadingTrailer
  };

class Comms : public QDialog
{
     Q_OBJECT

signals:
    void DataReady(void);
    void ADCrecordingDone(void);
    void ADCvectorReady(void);

public:
    explicit Comms(SettingsDialog *settings, QString Host, QStatusBar *statusbar);
    bool ConnectToMIPS();
    void DisconnectFromMIPS();
    bool SendCommand(QString name, QString message);
    bool SendCommand(QString message);
    QString SendMessage(QString name, QString message);
    QString SendMessage(QString message);
    QString SendMess(QString name, QString message);
    QString SendMess(QString message);
    void SendString(QString name, QString message);
    void SendString(QString message);
    void writeData(const QByteArray &data);
    void msDelay(int ms);
    bool openSerialPort();
    void reopenSerialPort(void);
    void closeSerialPort();
    void waitforline(int timeout);
    char getchar(void);
    void GetADCbuffer(quint16 *ADCbuffer, int NumSamples);
    void ADCrelease(void);
    void GetMIPSfile(QString MIPSfile, QString LocalFile);
    void PutMIPSfile(QString MIPSfile, QString LocalFile);
    void GetEEPROM(QString FileName, QString Board, int Addr);
    void PutEEPROM(QString FileName, QString Board, int Addr);
    void GetARBFLASH(QString FileName);
    void PutARBFLASH(QString FileName);
    void ARBupload(QString Faddress, QString FileName);
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

    // ADC buffer processing
    ADCreadStates ADCstate;
    quint16 *ADCbuf;
    int ADClen;
    QTimer *keepAliveTimer;

private slots:
    void handleError(QSerialPort::SerialPortError error);
    void readData2RingBuffer(void);
    void readData2ADCBuffer(void);
    void connected(void);
    void disconnected(void);
    void slotAboutToClose(void);
    void slotKeepAlive(void);
};

#endif // COMMS

