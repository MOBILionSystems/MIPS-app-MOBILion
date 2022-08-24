#include "qtofaddonclient.h"
#include <QtMath>
#include <QTimer>

QtofAddonClient::QtofAddonClient(QObject *parent)
    : QObject{parent}
{

}

int32_t float_to_fixedpoint(float value){
    /// Given a floating point value, return a Fixs32en20
    auto scaled = (float) (value * pow(2.0, 20.0));
    return (int32_t) scaled;
}

void QtofAddonClient::applyCeVoltage(QString ip, int voltage)
{
    qDebug() << ip << ", " << voltage;
    response.clear();
    _voltage = voltage;
    socket = new QTcpSocket(this);

    QObject::connect(socket, SIGNAL(connected()),this, SLOT(onConnected()));
    QObject::connect(socket, SIGNAL(disconnected()),this, SLOT(onDisconnected()));
    QObject::connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(onBytesWritten(qint64)));
    QObject::connect(socket, SIGNAL(readyRead()),this, SLOT(onReadyRead()));

    qDebug() << "connecting...";

    // this is not blocking call
    socket->connectToHost(ip, 8080);

    // we need to wait...
    if(!socket->waitForConnected(5000))
    {
        qDebug() << "Error: " << socket->errorString();
        emit ceVoltageReceived(false);
    }
}

void QtofAddonClient::onConnected()
{
    qDebug() << "connected...";
    qDebug() << "applyCeVoltage";
    ssize_t payload_length = sizeof(collision_energy_profile_t);
    //operation_t operation = Noop;
    QByteArray operation;
    operation.resize(2);
    operation[0] = 0x01;
    operation[1] = 0x00;
    collision_energy_profile_t payloadorigin;
    payloadorigin.collision_energy_interval_us = 1000;
    payloadorigin.collision_energy[0].interval_count = 1;
    payloadorigin.collision_energy[0].collision_energy_v = float_to_fixedpoint(_voltage);

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

void QtofAddonClient::onDisconnected()
{
    qDebug() << "disconnected...";

    QByteArray statusArray = response.left(2);
    qDebug() << "status: " << statusArray;

    response.remove(0, 2);
    QString description(response);
    qDebug() << "description: " << description;
    if(description.contains("Collision Profile received, processing")){
        emit ceVoltageReceived(true);
    }else{
        emit ceVoltageReceived(false);
    }
    response.clear();
}

void QtofAddonClient::onBytesWritten(qint64 bytes)
{
    qDebug() << bytes << " bytes written...";
}

void QtofAddonClient::onReadyRead()
{
    qDebug() << "reading...";

    QByteArray thisResponse = socket->readAll();
    if(!thisResponse.isEmpty())
        response.append(thisResponse);
}

