#ifndef DATAPROCESS_H
#define DATAPROCESS_H

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

class DataProcess : public QObject
{
    Q_OBJECT
public:
    explicit DataProcess(QObject *parent = nullptr);

    double sumProcess(QJsonArray data);

signals:

};

#endif // DATAPROCESS_H
