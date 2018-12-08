#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QDebug>
#include <QStatusBar>

#include "ringbuffer.h"

class TCPserver : public QObject
{
    Q_OBJECT
public:
    explicit TCPserver(QObject *parent = 0);
    ~TCPserver();
    int bytesAvailable(void);
    void sendMessage(QString mess);
    QString readLine(void);
    void listen(void);
    int  port;
    QStatusBar *statusbar;

signals:
    void dataReady(void);
    void lineReady(void);

public slots:
    void newConnection(void);
    void readData(void);
    void disconnected(void);

private:
    RingBuffer rb;
    QTcpServer *server;
    QTcpSocket *socket;
};

#endif // TCPSERVER_H
