#ifndef COMMANDGENERATOR_H
#define COMMANDGENERATOR_H

#include <QObject>
#include <QJsonObject>
#include <QUuid>
#include "common.h"

class CommandGenerator : public QObject
{
    Q_OBJECT
public:
    explicit CommandGenerator(QObject *parent = nullptr);
    QJsonObject getCommand(CommandType type);

signals:

private:
    QString talismanUUID;
    unsigned int sequence = 1;
    void initCommand(QJsonObject& command);
    void updateInitDigtizerCommand(QJsonObject& command);
    void updateStartAcqCommand(QJsonObject& command);
    void updateStopAcqCommand(QJsonObject& command);

    QString getUUID();

};

#endif // COMMANDGENERATOR_H
