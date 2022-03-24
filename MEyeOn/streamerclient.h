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


signals:
    void messageReady(QString message);

};

#endif // STREAMERCLIENT_H
