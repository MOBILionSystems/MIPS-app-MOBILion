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
    command.insert("sequence", QJsonValue(QString::number(sequence)));
}

void CommandGenerator::updateInitDigtizerCommand(QJsonObject &command)
{
    QJsonArray target;
    target.append("ACORN_ACQUIRE");
    command.insert("target", QJsonValue(target));

    command.insert("command", QJsonValue("CONFIGURE_DIGITIZER"));

    QJsonObject initDigitizer;
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
    QJsonArray target;
    target.append("ACORN_ACQUIRE");
    target.append("ACORN_ARCHIVE");
    command.insert("target", QJsonValue(target));

    command.insert("command", QJsonValue("START_ACQUISITION"));
    command.insert("talisman", "906b3a6910d84259a810e9d8c35a564f");

    QJsonObject data;

    data.insert("filename", QJsonValue("/acorn/data/20220228/b.mbi"));

    QJsonObject frameObject;
    frameObject.insert("frm-metadata-id",  QJsonValue("1"));
    frameObject.insert("frm-num-bin-ms",  QJsonValue("320000"));
    frameObject.insert("frm-num-bin-dt",  QJsonValue("10000"));
    frameObject.insert("frm-mux-gate",  QJsonValue("0"));
    frameObject.insert("frm-method-state", QJsonValue("")); //too much
    frameObject.insert("frm-method-name",  QJsonValue("TuneMixTwAmp30"));
    frameObject.insert("frm-timing-intents", QJsonValue("")); //too much
    frameObject.insert("frm-polarity", "Negative");

    frameObject.insert("cal-dt-polynomial", QJsonValue(""));
    frameObject.insert("cal-dt-power-flags", QJsonValue(""));
    frameObject.insert("cal-dt-traditional", QJsonValue(""));
    frameObject.insert("cal_ms-polynomial", QJsonValue(""));
    frameObject.insert("cal-ms-power-flags", QJsonValue(""));

    QJsonObject msTraditional;
    msTraditional.insert("slope", QJsonValue("0.345"));
    msTraditional.insert("intercept", QJsonValue("-3.913"));
    QJsonArray mzResidual;
    mzResidual.insert(0, QJsonValue("1537.855"));
    mzResidual.insert(0, QJsonValue("-73.017"));
    mzResidual.insert(0, QJsonValue("1.36"));
    mzResidual.insert(0, QJsonValue("-0.012"));
    mzResidual.insert(0, QJsonValue("0.000056"));
    mzResidual.insert(0, QJsonValue("-9.96e-8"));
    msTraditional.insert("mz_residual_terms", QJsonValue(mzResidual));

    frameObject.insert("cal-ms-traditional", QJsonValue(""));

    QJsonArray frameArray;
    frameArray.insert(0, QJsonValue(frameObject));

    data.insert("frame", frameArray);

    QJsonObject global;

    global.insert("smp-type", QJsonValue("Calibration Tuning Mix"));
    global.insert("smp-position", QJsonValue("p1-a1"));
    global.insert("smp-concentration", QJsonValue("50.0"));
    global.insert("smp-injection-volume", QJsonValue(""));
    global.insert("smp-plate-code", QJsonValue(""));
    global.insert("smp-rack-code", QJsonValue(""));
    global.insert("smp-dilution-factor", QJsonValue(""));
    global.insert("smp-plate-position", QJsonValue(""));
    global.insert("smp-balance-type", QJsonValue(""));
    global.insert("usr-group", QJsonValue("MOBILIon Systems, Inc"));
    global.insert("usr-role", QJsonValue("Operator"));
    global.insert("usr-name", QJsonValue("service"));

    global.insert("adc-baseline-stabilize-enable", QJsonValue("1"));
    global.insert("adc-channel", QJsonValue("Channel1"));
    global.insert("adc-digital-offset", QJsonValue("-31456"));
    global.insert("adc-driver-rev", QJsonValue("AqMD3-3.6.7279.14 (Acqiris: SA220P)"));
    global.insert("adc-firmware-rev", QJsonValue("Stage1 Version 3.7.173.0, FPGA Firmware Version 3.7.173.69125/AVG.CST.DGT, FE CPLD 1/0.4.0.0"));
    global.insert("adc-mass-spec-range", QJsonValue("3200"));
    global.insert("adc-min-nanoseconds", QJsonValue("15000"));
    global.insert("adc-model", QJsonValue("SA220P"));
    global.insert("adc-offset", QJsonValue("0.24"));
    global.insert("adc-pulse-threshold", QJsonValue("200"));
    global.insert("adc-range", QJsonValue("0.5"));
    global.insert("adc-record-size", QJsonValue("320000"));
    global.insert("adc-sample-rate", QJsonValue("2.0e+09"));
    global.insert("adc-self-trigger-duty-cycle", QJsonValue("10.000000"));
    global.insert("adc-self-trigger-enable", QJsonValue("0"));
    global.insert("adc-self-trigger-frequency", QJsonValue("10000.000000"));
    global.insert("adc-self-trigger-polarity", QJsonValue("1"));
    global.insert("adc-trigger-level", QJsonValue("1.250000"));
    global.insert("adc-trigger-polarity", QJsonValue("1"));
    global.insert("adc-zero-value", QJsonValue("-31456"));
    global.insert("adc-zs-hysteresis", QJsonValue("100"));
    global.insert("adc-zs-postgate-samples", QJsonValue("0"));
    global.insert("adc-zs-pregate-samples", QJsonValue("0"));
    global.insert("adc-zs-threshold", QJsonValue("-29706"));


    // stuck with error
//    QJsonObject acqBoardT; acqBoardT.insert("post", "59.96"); acqBoardT.insert("pre", "59.93");
//    global.insert("acq-board-temp", QJsonValue(acqBoardT));
    global.insert("acq-Ic-model", QJsonValue("Agilent 1290 Infinity II"));
    global.insert("acq-ms-method", QJsonValue(""));
    global.insert("acq-ms-model", QJsonValue("Agilent 6545XT"));
    global.insert("acq-num-frames", QJsonValue(""));
    global.insert("acq-slim_path_length", QJsonValue(""));
    global.insert("acq_software-version", QJsonValue("release-v1.0.27"));
//    QJsonObject acqTempCorr; acqTempCorr.insert("A", QJsonValue("0.0")); acqTempCorr.insert("B", QJsonValue("0.0"));
//    global.insert("acq_temperature_correction", QJsonValue(acqTempCorr));
    global.insert("acq-timestamp", QJsonValue("2022-02-10 16:32:33.977127+00:00"));
    global.insert("acq-tune-file", QJsonValue(""));
    global.insert("acq-type", QJsonValue("Sample"));
    global.insert("acq-vendor-metadata", QJsonValue("906b3a6910d84259a810e9d8c35a564f"));

    data.insert("global", global);

    command.insert("data", data);
}

void CommandGenerator::updateStopAcqCommand(QJsonObject &command)
{
    QJsonArray target;
    target.append("ACORN_ACQUIRE");
    command.insert("target", QJsonValue(target));
    command.insert("command", QJsonValue("STOP_ACQUISITION"));
    command.insert("talisman", "906b3a6910d84259a810e9d8c35a564f");
}
