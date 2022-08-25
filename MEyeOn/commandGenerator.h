#ifndef COMMANDGENERATOR_H
#define COMMANDGENERATOR_H

#include <QObject>
#include <QJsonObject>
#include <QUuid>
#include "common.h"
#include <QHash>

class CommandGenerator : public QObject
{
    Q_OBJECT
public:
    explicit CommandGenerator(QObject *parent = nullptr);
    QJsonObject getCommand(CommandType type, QString fileName = "", bool maf = false, int ceVoltage = 30);
    static QString getUUID();
    QString currentTalismanUUID() const;
    unsigned int lastUsedSequency() const;
    static QHash<QString, QJsonValue> adcMap;
    static QHash<QString, QJsonValue> smpMap;
    static QHash<QString, QJsonValue> usrMap;
    static QHash<QString, QJsonValue> acqMap;
    static QHash<QString, QJsonValue> frmMap;
    static QString ceProfile;

    void updateInfo(QString key, QString value);

signals:

private:

    QString talismanUUID;
    unsigned int sequence = 1;
    void initCommand(QJsonObject& command);
    void updateInitDigtizerCommand(QJsonObject& command);
    void updateStartAcqCommand(QJsonObject& command, QString fileName, bool maf, int ceVoltage);
    void updateStopAcqCommand(QJsonObject& command);
};

#endif // COMMANDGENERATOR_H
