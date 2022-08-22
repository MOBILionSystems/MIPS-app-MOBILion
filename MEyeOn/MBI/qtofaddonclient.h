#ifndef QTOFADDONCLIENT_H
#define QTOFADDONCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QDebug>

typedef int32_t Fixs32en20_t;
typedef Fixs32en20_t fixed_point_t;
typedef __int64 ssize_t;
typedef unsigned char	u_char;

struct collision_energy_frame_profile_t {
    uint32_t interval_count;  // # of times to repeat this setting
    fixed_point_t collision_energy_v;  // fragmentation voltage, can be negative!
};


struct collision_energy_profile_t {
    uint32_t collision_energy_interval_us;
    collision_energy_frame_profile_t collision_energy[1000];
};

class QtofAddonClient : public QObject
{
    Q_OBJECT
public:
    explicit QtofAddonClient(QObject *parent = nullptr);

    void doConnect(QString ip);
    void applyCeVoltage(int voltage);

signals:

public slots:
    void connected();
    void disconnected();
    void bytesWritten(qint64 bytes);
    void readyRead();

private:
    QTcpSocket *socket;
    QString ip;
    bool isConnected = false;

};

#endif // QTOFADDONCLIENT_H
