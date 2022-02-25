#include "commandGenerator.h"
#include <QJsonArray>
#include <QJsonValue>

CommandGenerator::CommandGenerator(QObject *parent)
    : QObject{parent}
{

}

QJsonObject CommandGenerator::getCommand(CommandType type)
{
    QJsonObject command;
    initCommand(command);
    switch (type) {
    case CommandType::Initialization:
        updateInitDigtizerCommand(command);
        break;
    case CommandType::Start_Acquisition:
        updateStartAcqCommand(command);
        break;
    case CommandType::Stop_Acquisition:
        updateStopAcqCommand(command);
        break;
    }
    sequence++;
    return command;
}

void CommandGenerator::initCommand(QJsonObject &command)
{
    QJsonArray target;
    target.append("ACORN_ACQUIRE");
    command.insert("target", QJsonValue(target));
    command.insert("command", QJsonValue("CONFIGURE_DIGITIZER"));
    command.insert("sequence", QJsonValue(QString::number(sequence)));
    command.insert("data", QJsonValue("{}"));
}

void CommandGenerator::updateInitDigtizerCommand(QJsonObject &command)
{
    QJsonObject initDigitizer;
    if(command.contains("data"))
        initDigitizer = command.take("data").toObject();

    initDigitizer.insert("adc-baseline-stabilize-enable", QJsonValue("1"));
    initDigitizer.insert("adc-channel", QJsonValue("Channel1"));
    initDigitizer.insert("adc-digital-offset", QJsonValue("-31456"));
    initDigitizer.insert("adc-driver-rev", QJsonValue("AqMD3-3.6.7279.14 (Acqiris: SA220P)"));
    initDigitizer.insert("adc-firmware-rev", QJsonValue("Stage1 Version 3.7.173.0, FPGA Firmware Version 3.7.173.69125/AVG.CST.DGT, FE CPLD 1/0.4.0.0"));
    initDigitizer.insert("adc-mass-spec-range", QJsonValue("3200"));
    initDigitizer.insert("adc-min-nanoseconds", QJsonValue("15000"));
    initDigitizer.insert("adc-model", QJsonValue("SA220P"));
    initDigitizer.insert("adc-offset", QJsonValue("0.24"));
    initDigitizer.insert("adc-pulse-threshold", QJsonValue("200"));
    initDigitizer.insert("adc-range", QJsonValue("0.5"));
    initDigitizer.insert("adc-record-size", QJsonValue("320000"));
    initDigitizer.insert("adc-sample-rate", QJsonValue("2.0e+09"));
    initDigitizer.insert("adc-self-trigger-duty-cycle", QJsonValue("10.000000"));
    initDigitizer.insert("adc-self-trigger-enable", QJsonValue("0"));
    initDigitizer.insert("adc-self-trigger-frequency", QJsonValue("10000.000000"));
    initDigitizer.insert("adc-self-trigger-polarity", QJsonValue("1"));
    initDigitizer.insert("adc-trigger-level", QJsonValue("1.250000"));
    initDigitizer.insert("adc-trigger-polarity", QJsonValue("1"));
    initDigitizer.insert("adc-zero-value", QJsonValue("-31456"));
    initDigitizer.insert("adc-zs-hysteresis", QJsonValue("100"));
    initDigitizer.insert("adc-zs-postgate-samples", QJsonValue("0"));
    initDigitizer.insert("adc-zs-pregate-samples", QJsonValue("0"));
    initDigitizer.insert("adc-zs-threshold", QJsonValue("-29706"));

    command.insert("data", initDigitizer);
}

void CommandGenerator::updateStartAcqCommand(QJsonObject &command)
{

}

void CommandGenerator::updateStopAcqCommand(QJsonObject &command)
{

}
