#include "comms.h"

Comms::Comms(SettingsDialog *settings, QString Host, QStatusBar *statusbar)
{
    p = settings->settings();
    sb = statusbar;
    host = Host;
    serial = new QSerialPort(this);
    connect(&client, SIGNAL(readyRead()), this, SLOT(readData2RingBuffer()));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData2RingBuffer()));
    connect(&client, SIGNAL(connected()),this, SLOT(connected()));
    connect(&client, SIGNAL(disconnected()),this, SLOT(disconnected()));
}

void Comms::SendString(QString message)
{
    if (!serial->isOpen() && !client.isOpen())
    {
        sb->showMessage("Disconnected!",2000);
        return;
    }
    if (serial->isOpen()) serial->write(message.toStdString().c_str());
    if (client.isOpen()) client.write(message.toStdString().c_str());
}

void Comms::SendCommand(QString message)
{
    QString res;

    if (!serial->isOpen() && !client.isOpen())
    {
        sb->showMessage("Disconnected!",2000);
        return;
    }
    for(int i=0;i<2;i++)
    {
        rb.clear();
        if (serial->isOpen()) serial->write(message.toStdString().c_str());
        if (client.isOpen()) client.write(message.toStdString().c_str());
        rb.waitforline(1000);
        if(rb.numLines() >= 1)
        {
            res = rb.getline();
            if(res == "") return;
            if(res == "?")
            {
                res = message + " :NAK";
                sb->showMessage(res.toStdString().c_str(),2000);
                return;
            }
        }
    }
    res = message + " :Timeout";
    sb->showMessage(res.toStdString().c_str(),2000);
    return;
}

QString Comms::SendMessage(QString message)
{
    QString res;

    if (!serial->isOpen() && !client.isOpen())
    {
        sb->showMessage("Disconnected!",2000);
        return "";
    }
     for(int i=0;i<2;i++)
    {
        rb.clear();
        if (serial->isOpen()) serial->write(message.toStdString().c_str());
        if (client.isOpen()) client.write(message.toStdString().c_str());
        rb.waitforline(500);
        if(rb.numLines() >= 1)
        {
            res = rb.getline();
            if(res != "") return res;
        }
    }
    res = message + " :Timeout";
    sb->showMessage(res.toStdString().c_str(),2000);
    res = "";
    return res;
}

bool Comms::ConnectToMIPS()
{
    QTime timer;
    QString res;

    if(client.isOpen() || serial->isOpen()) return true;

    if(host != "")
    {
       client_connected = false;
       client.connectToHost(host, 2015);
       sb->showMessage(tr("Connecting..."));
       timer.start();
       while(timer.elapsed() < 10000)
       {
           QApplication::processEvents();
           if(client_connected)
           {
               return true;
           }
       }
       sb->showMessage(tr("MIPS failed to connect!"));
       client.abort();
       client.close();
       return false;
    }
    else
    {
       openSerialPort();
       serial->setDataTerminalReady(true);
    }
    return true;
}

void Comms::DisconnectFromMIPS()
{
    if(client.isOpen()) client.close();
    closeSerialPort();
}

bool Comms::openSerialPort()
{
    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,SLOT(handleError(QSerialPort::SerialPortError)));
    serial->setPortName(p.name);
    #if defined(Q_OS_MAC)
        serial->setPortName("cu." + p.name);
    #endif
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite))
    {
        sb->showMessage(QString("Connected to %1 : %2, %3, %4, %5, %6")
                               .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                               .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        return true;
    }
    else
    {
        QMessageBox::critical(this, QString("Error"), serial->errorString());
        sb->showMessage(QString("Open error"));
    }
    return false;
}

void Comms::closeSerialPort()
{
    if (serial->isOpen()) serial->close();
    serial->close();
    serial->close();
    serial->close();
    sb->showMessage(tr("Disconnected"));
    disconnect(serial, SIGNAL(error(QSerialPort::SerialPortError)),0,0);
}

void Comms::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

void Comms::writeData(const QByteArray &data)
{
    if(client.isOpen()) client.write(data);
    if(serial->isOpen()) serial->write(data);
}

void Comms::readData2RingBuffer(void)
{
    int i;

    if(client.isOpen())
    {
        QByteArray data = client.readAll();
        for(i=0;i<data.size();i++) rb.putch(data[i]);
    }
    if(serial->isOpen())
    {
        QByteArray data = serial->readAll();
        for(i=0;i<data.size();i++) rb.putch(data[i]);
    }
    emit DataReady();
}

void Comms::connected(void)
{
    sb->showMessage(tr("MIPS connected"));
    client_connected = true;
}

void Comms::disconnected(void)
{
    sb->showMessage(tr("Disconnected"));
}

void Comms::waitforline(int timeout)
{
    rb.waitforline(timeout);
}

QString Comms::getline(void)
{
    return(rb.getline());
}

QByteArray Comms::readall(void)
{
    QByteArray data;
    char c;

    while(1)
    {
      c = rb.getch();
      if(c==0) return data;
      data += c;
    }
}
