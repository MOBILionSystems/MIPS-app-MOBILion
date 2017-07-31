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

char Comms::getchar(void)
{
    return rb.getch();
}

int Comms::CalculateCRC(QByteArray fdata)
{
    unsigned char generator = 0x1D;
    unsigned char crc = 0;

    for(int i=0; i<fdata.count(); i++)
    {
      crc ^= fdata[i];
      for(int j=0; j<8; j++)
      {
        if((crc & 0x80) != 0)
        {
          crc = ((crc << 1) ^ generator);
        }
        else
        {
          crc <<= 1;
        }
      }
    }
    return crc;
}

void Comms::GetMIPSfile(QString MIPSfile, QString LocalFile)
{
    bool ok = false;
    QString FileData;
    char c;
    int len;

    // Send the command to MIPS to get the file
    if(SendCommand("GET," + MIPSfile + "\n"))
    {
       // Now process the data in the ring buffer
       // First read the length
        waitforline(500);
        QString FileSize = getline();
        // Read the hex data stream
        for(int i=0; i<FileSize.toInt(); i+=1024)
        {
            len = 1024;
            if((FileSize.toInt() - i) < 1024) len = FileSize.toInt() - i;
            // Read the block of data
            for(int j=0; j<len*2; j++)
            {
              while((c = getchar()) == 0) QApplication::processEvents();
              if(c == '\n') break;
              FileData += QChar(c);
            }
            // Send "Next" block request
            if((FileSize.toInt()-i) > 1024) SendString("Next\n");
        }
        while((c = getchar()) == 0) QApplication::processEvents();
        if(FileData.count() != 2 * FileSize.toInt())
        {
            // Here if file data block is not the proper size
            QMessageBox::critical(this, tr("File Error"), "Data block size not correct!");
            return;
        }
        // Read the CRC
        waitforline(500);
        QString FileCRC = getline();
        // Convert data to byte array and callculate CRC
        QByteArray fdata;
        fdata.resize(FileSize.toInt());
        for(int i=0; i<FileSize.toInt(); i++)
        {
            fdata[i] = FileData.mid(i*2,2).toUInt(&ok,16);
        }
        if(CalculateCRC(fdata) == FileCRC.toInt())
        {
            // Now save the data to the user selected file
            QFile file(LocalFile);
            file.open(QIODevice::WriteOnly);
            file.write(fdata);
            file.close();
            QMessageBox::information(this, tr("File saved"), "File from MIPS read and saved successfully!");
        }
        else
        {
            // Here with file CRC error, raise error message
            QMessageBox::critical(this, tr("File Error"), "CRC error!");
        }
    }
}

void Comms::PutMIPSfile(QString MIPSfile, QString LocalFile)
{
    QByteArray fdata;
    int fsize,len;
    QString dblock,res;

    // open the local file to send and read the data
    QFile file(LocalFile);
    file.open(QIODevice::ReadOnly);
    fdata = file.readAll();
    // Convert the data into a ascii hex string
    for(int i=0; i<file.size(); i++)
    {
         dblock += QString().sprintf("%02x",(unsigned char)fdata[i]);
    }
    fsize = file.size();
    file.close();
    // Send the data to MIPS
    if(SendCommand("PUT," + MIPSfile + "," + QString().number(fsize) + "\n"))
    {
        for(int i=0; i<dblock.count(); i+=1024)
        {
            len = 1024;
            if((dblock.count() - i) < 1024) len = dblock.count() - i;
            SendString(dblock.mid(i,len));
            if(len == 1024)
            {
                waitforline(1000);
                if((res = getline()) == "")
                {
                    // Here if we timed out, display error and exit
                    QMessageBox::critical(this, tr("Data read error"), "Timedout waiting for data from MIPS!");
                    return;
                }
            }
            QApplication::processEvents();
        }
        SendString("\n");
        SendString(QString().number(CalculateCRC(fdata)) + "\n");
        QMessageBox::information(this, tr("File saved"), "File sent to MIPS!");
    }
}

void Comms::GetEEPROM(QString FileName, QString Board, int Addr)
{
    bool ok = false;
    QString FileData;

    // Send the command to MIPS to get the file
    if(SendCommand("GETEEPROM," + Board + "," + QString().sprintf(",%x\n",Addr)))
    {
       // Now process the data in the ring buffer
       // First read the length
        waitforline(500);
        QString FileSize = getline();
        // Read the hex data stream
        waitforline(1000);
        FileData = getline();
        if(FileData.count() != 2 * FileSize.toInt())
        {
            // Here if file data block is not the proper size
            QMessageBox::critical(this, tr("File Error"), "Data block size not correct!");
            return;
        }
        // Read the CRC
        waitforline(500);
        QString FileCRC = getline();
        // Convert data to byte array and callculate CRC
        QByteArray fdata;
        fdata.resize(FileSize.toInt());
        for(int i=0; i<FileSize.toInt(); i++)
        {
            fdata[i] = FileData.mid(i*2,2).toUInt(&ok,16);
        }
        if(CalculateCRC(fdata) == FileCRC.toInt())
        {
            // Now save the data to the user selected file
            QFile file(FileName);
            file.open(QIODevice::WriteOnly);
            file.write(fdata);
            file.close();
            QMessageBox::information(this, tr("File saved"), "EEPROM from MIPS read and saved successfully!");
        }
        else
        {
            // Here with file CRC error, raise error message
            QMessageBox::critical(this, tr("File Error"), "CRC error!");
        }
    }
}

void Comms::PutEEPROM(QString FileName, QString Board, int Addr)
{
    bool ok = false;
    QString FileData;
    QString dblock;
    QByteArray fdata;

    // Send the command to MIPS to get the file
    if(SendCommand("PUTEEPROM," + Board + QString().sprintf(",%x\n",Addr)))
    {
        // open the local file to send and read the data
        QFile file(FileName);
        file.open(QIODevice::ReadOnly);
        fdata = file.readAll();
        // Convert the data into a ascii hex string
        for(int i=0; i<file.size(); i++)
        {
             dblock += QString().sprintf("%02x",(unsigned char)fdata[i]);
        }
        //SendCommand("\n");
        SendString(QString().number(file.size()) + "\n");
        file.close();
        // Send the data to MIPS
        SendString(dblock + "\n");
        SendString(QString().number(CalculateCRC(fdata)) + "\n");
        QMessageBox::information(this, tr("EEPROM write"), "MIPS module's EEPROM Written!");
    }
}

void Comms::SendString(QString name, QString message)
{
    if((name == MIPSname) || (name == "")) SendString(message);
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

bool Comms::SendCommand(QString name, QString message)
{
    if((name == MIPSname) || (name == "")) return(SendCommand(message));
    return false;
}

bool Comms::SendCommand(QString message)
{
    QString res;

    if (!serial->isOpen() && !client.isOpen())
    {
        sb->showMessage("Disconnected!",2000);
        return true;
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
            if(res == "") return true;
            if(res == "?")
            {
                res = message + " :NAK";
                sb->showMessage(res.toStdString().c_str(),2000);
                return false;
            }
        }
    }
    res = message + " :Timeout";
    sb->showMessage(res.toStdString().c_str(),2000);
    return true;
}

QString Comms::SendMessage(QString name, QString message)
{
    if((name == MIPSname) || (name == "")) return SendMessage(message);
    return "";
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
        rb.waitforline(1000);
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

// Open connection to MIPS and read its name.
bool Comms::ConnectToMIPS()
{
    QTime timer;
    QString res;

    MIPSname = "";
    if(client.isOpen() || serial->isOpen()) return true;

    if(host != "")
    {
       client_connected = false;
       client.connectToHost(host, 2015);
       sb->showMessage(tr("Connecting..."));
       timer.start();
       while(timer.elapsed() < 30000)
       {
           QApplication::processEvents();
           if(client_connected)
           {
               MIPSname = SendMessage("GNAME\n");
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
       MIPSname = SendMessage("GNAME\n");
    }
    return true;
}

void Comms::DisconnectFromMIPS()
{
    if(client.isOpen()) client.close();
    closeSerialPort();
}

bool Comms::isMIPS(QString port)
{
    QString res;

    if(serial->isOpen()) return false;  // If the port is open then return false
    disconnect(serial, SIGNAL(error(QSerialPort::SerialPortError)),0,0);
    // Init the port parameters
    p.name = port;
    serial->setPortName(p.name);
#if defined(Q_OS_MAC)
    serial->setPortName("cu." + p.name);
#endif
    if(serial->isOpen()) return false;
    p.baudRate = QSerialPort::Baud115200;
    p.dataBits = QSerialPort::Data8;
    p.parity = QSerialPort::NoParity;
    p.stopBits = QSerialPort::OneStop;
    p.flowControl = QSerialPort::NoFlowControl;
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite))
    {
        serial->setDataTerminalReady(true);  // Required on PC but not on a MAC
        QApplication::processEvents();
        res = SendMessage("GVER\n");
        serial->close();
        serial->close();
        serial->close();
        serial->close();
        serial->clearError();
        QApplication::processEvents();
        rb.clear();
        if(res.contains("Version")) return true;
    }
    serial->clearError();
    rb.clear();
    return false;
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

bool Comms::isConnected(void)
{
    if(client_connected) return true;
    if(serial->isOpen()) return true;
    return false;
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
