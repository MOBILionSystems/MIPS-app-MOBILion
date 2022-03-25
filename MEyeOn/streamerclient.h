#ifndef STREAMERCLIENT_H
#define STREAMERCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>

class StreamerClient : public QObject
{
    Q_OBJECT
public:
    explicit StreamerClient(QObject *parent = nullptr);
    void connectTo();
    void request(QString message);

private slots:
    void OnConnected();
    void onDisconnected();
    void onBytesWritten(qint64 bytes);
    void readyRead(QString message = "");

private:
    QWebSocket* m_webSocket;
    QString guid;
    int frameindex = 0;
    const QString streamGUID = "GUID-4b5d4bce081e365cd2eb848e3b41e31b56d16bc780f7d559a3ae629edd2e1000";
    const QString stream1GUID = "GUID-4b5d4bce081e365cd2eb848e3b41e31b56d16bc780f7d559a3ae629edd2e1444";
    const QString stream2GUID = "GUID-4b5d4bce081e365cd2eb848e3b41e31b56d16bc780f7d559a3ae629edd2e1555";
    const QString stream3GUID = "GUID-4b5d4bce081e365cd2eb848e3b41e31b56d16bc780f7d559a3ae629edd2e1666";
    const QString mobilityGUID = "GUID-4b5d4bce081e365cd2eb848e3b41e31b56d16bc780f7d559a3ae629edd2e1777";
    const QString heatmapGUID = "GUID-4b5d4bce081e365cd2eb848e3b41e31b56d16bc780f7d559a3ae629edd2e1888";


signals:
    void messageReady(QString message);

};

#endif // STREAMERCLIENT_H
