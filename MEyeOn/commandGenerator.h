#ifndef COMMANDGENERATOR_H
#define COMMANDGENERATOR_H

#include <QObject>
#include <QJsonObject>
#include "common.h"

class CommandGenerator : public QObject
{
    Q_OBJECT
public:
    explicit CommandGenerator(QObject *parent = nullptr);
    QJsonObject getCommand(CommandType type);

signals:

private:
    unsigned int sequence = 1;
    void initCommand(QJsonObject& command);
    void updateInitDigtizerCommand(QJsonObject& command);
    void updateStartAcqCommand(QJsonObject& command);
    void updateStopAcqCommand(QJsonObject& command);

};

#endif // COMMANDGENERATOR_H
