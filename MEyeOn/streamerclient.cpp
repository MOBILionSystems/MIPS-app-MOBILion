#include "streamerclient.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>


StreamerClient::StreamerClient(QObject *parent)
    : QObject{parent}
{
    m_webSocket = new QWebSocket();
    connect(m_webSocket, &QWebSocket::connected, this, &StreamerClient::OnConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &StreamerClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this,&StreamerClient::readyRead);
    connect(m_webSocket, &QWebSocket::bytesWritten, this, &StreamerClient::onBytesWritten);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &StreamerClient::messageReady);
}

void StreamerClient::connectTo(QString sbcStreamer)
{
    //m_webSocket->open(QUrl(QString("ws://%1").arg(sbcStreamer))); //ws://192.168.1.212:4001
    m_webSocket->open(QUrl(QString("ws://192.168.1.213:4001")));
}

// REQUEST_FULL_SPECTRUM, REQUEST_DATA_STREAM(MASS),
void StreamerClient::request(QString message)
{
    //message = "{\"dataDomainWindow\": {\"xRange\": [0, 250000], \"yRange\": [0, 300000]}, \"id\": \"REQUEST_DATA_STREAM\", \"guid\": \"" + guid + "\", \"streamGuid\": \"" + heatmapGUID + "\", \"type\": \"HEATMAP\", \"xAxisPixels\": 100000, \"yAxisPixels\": 10000}";
    //message = "{\"id\": \"REQUEST_DATA_STREAM\", \"guid\": \"" + guid + "\", \"streamGuid\": \"" + heatmapGUID + "\", \"type\": \"HEATMAP\", \"xAxisPixels\": 100000, \"yAxisPixels\": 10000}";
    //message = "{\"dataDomainWindow\": {\"xRange\": [0, 250000], \"yRange\": [0, 300000]}, \"id\": \"REQUEST_DATA_STREAM\", \"guid\": \"" + guid + "\", \"streamGuid\": \"" + mobilityGUID + "\", \"type\": \"MOBILITY\", \"xAxisPixels\": 100000, \"yAxisPixels\": 10000}";
    //message = "{\"dataDomainWindow\": {\"xRange\": [0, 250000], \"yRange\": [0, 300000]}, \"id\": \"REQUEST_DATA_STREAM\", \"guid\": \"" + guid + "\", \"streamGuid\": \"" + streamGUID + "\", \"type\": \"MASS\", \"xAxisPixels\": 100000, \"yAxisPixels\": 10000}";
    QString message1 = "{\"id\": \"REQUEST_DATA_STREAM\", \"guid\": \"" + guid + "\", \"streamGuid\": \"" + streamGUID + "\", \"type\": \"MASS\", \"xAxisPixels\": 631, \"yAxisPixels\": 381}";
    QString message2 = "{\"id\": \"REQUEST_DATA_STREAM\", \"guid\": \"" + guid + "\", \"streamGuid\": \"" + mobilityGUID + "\", \"type\": \"MOBILITY\", \"xAxisPixels\": 631, \"yAxisPixels\": 381}";
    QString message3 = "{\"id\": \"REQUEST_DATA_STREAM\", \"guid\": \"" + guid + "\", \"streamGuid\": \"" + heatmapGUID + "\", \"type\": \"HEATMAP\", \"xAxisPixels\": 551, \"yAxisPixels\": 341}";
    m_webSocket->sendTextMessage(message1);
    m_webSocket->sendTextMessage(message2);
    m_webSocket->sendTextMessage(message3);
}

void StreamerClient::resetFrameIndex()
{
    frameindex = 0;
}

void StreamerClient::OnConnected()
{
    qDebug() << "Connected!";
}

void StreamerClient::onDisconnected()
{
    qDebug() << "Disconnected!";
}

void StreamerClient::onBytesWritten(qint64 bytes)
{
    qDebug() << "We wrote: " << bytes;
}

void StreamerClient::readyRead(QString message)
{
    //qDebug() << "Reading..." << ++frameindex;
    QJsonDocument document = QJsonDocument::fromJson(message.toLocal8Bit());
    QJsonObject object = document.object();
    if(object.value("id").toString() == "CONNECT"){
        guid = document.object().value("payload").toObject().value("guid").toString();
        qDebug() << "guid: " << guid;
    }
}

//{\"dataDomainWindow\": {\"xRange\": [0, 250000], \"yRange\": [0, 300000]}, \"id\": \"REQUEST_DATA_STREAM\", \"guid\": \"GUID-5b8145ca74985aa9c2b310a9f804f3b29107e8b15fe43d53e5d25abab7edf21e\", \"streamGuid\": \"GUID-4b5d4bce081e365cd2eb848e3b41e31b56d16bc780f7d559a3ae629edd2e1000\", \"type\": \"MASS\", \"xAxisPixels\": 100000, \"yAxisPixels\": 10000}

