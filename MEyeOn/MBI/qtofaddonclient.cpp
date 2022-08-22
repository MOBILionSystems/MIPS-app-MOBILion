#include "qtofaddonclient.h"
#include <QtMath>

QtofAddonClient::QtofAddonClient(QObject *parent)
    : QObject{parent}
{

}

int32_t float_to_fixedpoint(float value){
    /// Given a floating point value, return a Fixs32en20
    auto scaled = (float) (value * pow(2.0, 20.0));
    return (int32_t) scaled;
}

void QtofAddonClient::doConnect(QString ip)
{
    socket = new QTcpSocket(this);

    QObject::connect(socket, SIGNAL(connected()),this, SLOT(connected()));
    QObject::connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    QObject::connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
    QObject::connect(socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

    qDebug() << "connecting...";

    // this is not blocking call
    socket->connectToHost(ip, 8080);

    // we need to wait...
    if(!socket->waitForConnected(5000))
    {
        qDebug() << "Error: " << socket->errorString();
    }
}

void QtofAddonClient::applyCeVoltage(int voltage)
{
    ssize_t payload_length = sizeof(collision_energy_profile_t);
    //operation_t operation = Noop;
    QByteArray operation;
    operation.resize(2);
    operation[0] = 0x01;
    operation[1] = 0x00;
    collision_energy_profile_t payloadorigin;
    payloadorigin.collision_energy_interval_us = 1000;
    payloadorigin.collision_energy[0].interval_count = 1;
    payloadorigin.collision_energy[0].collision_energy_v = float_to_fixedpoint(voltage);

    // Hey server, tell me about you.
    socket->write(operation);

    void* const payload = &payloadorigin;
    if (payload != nullptr) {  // we have a payload to send
        // chunk up the payload to test handling packet split across recv() calls
        ssize_t chunk_size = (ssize_t)payload_length / 10;
        for (ssize_t offset = 0; offset < payload_length; offset += chunk_size) {
            auto buffer = (u_char *)payload + offset;
            if ( (payload_length - offset) < chunk_size) {
                chunk_size = payload_length - offset;
            }
            socket->write((const char*)buffer, chunk_size);
        }
    }
}

void QtofAddonClient::connected()
{
    qDebug() << "connected...";
}

void QtofAddonClient::disconnected()
{
    qDebug() << "disconnected...";
}

void QtofAddonClient::bytesWritten(qint64 bytes)
{
    qDebug() << bytes << " bytes written...";
}

void QtofAddonClient::readyRead()
{
    qDebug() << "reading...";
    // read the data from the socket
    qDebug() << socket->readAll();
}

