#include "commandGenerator.h"
#include <QJsonArray>
#include <QJsonValue>
#include "dataprocess.h"

/*
QTOF Model	m/z Range (amu)	TOF Period (us)	Num Samples (SA220)	Valid Num Samples (SA220)
6545        1700            120               233000            232992
6545        3200            163.5             320000            320000
6545        10000       	288.5             570000            569984
6545XT      1700            120               233000            232992
6545XT      3200            163.5             320000            320000
6545XT      10000       	290.5             574000            573984
6545XT      30000       	501               995000            994976
6546        1700            107               207000            206976
6546        3200            146               285000            284992
6546        10000       	257.5             508000            508000
*/

QHash<QString, QJsonValue> CommandGenerator::adcMap{
    {"adc-baseline-stabilize-enable", QJsonValue("1")},
    {"adc-channel", QJsonValue("Channel1")},
    {"adc-digital-offset", QJsonValue("-31456")},
    {"adc-driver-rev", QJsonValue("AqMD3-3.6.7279.14 (Acqiris: SA220P)")},
    {"adc-firmware-rev", QJsonValue("Stage1 Version 3.7.173.0, FPGA Firmware Version 3.7.173.69125/AVG.CST.DGT, FE CPLD 1/0.4.0.0")},
    {"adc-mass-spec-range", QJsonValue("3200")},
    {"adc-min-nanoseconds", QJsonValue("15000")},
    {"adc-model", QJsonValue("SA220P")},
    {"adc-offset", QJsonValue("0.24")},
    {"adc-pulse-threshold", QJsonValue("200")},
    {"adc-range", QJsonValue("0.5")},
    {"adc-record-size", QJsonValue("284992")},
    {"adc-sample-rate", QJsonValue("2.0e+09")},
    {"adc-self-trigger-duty-cycle", QJsonValue("10.000000")},
    {"adc-self-trigger-enable", QJsonValue("0")},
    {"adc-self-trigger-frequency", QJsonValue("10000.000000")},
    {"adc-self-trigger-polarity", QJsonValue("1")},
    {"adc-trigger-level", QJsonValue("1.25")},          // 1.25 is for A2RAD (2.5v)
    {"adc-trigger-polarity", QJsonValue("1")},
    {"adc-zero-value", QJsonValue("-31456")},
    {"adc-zs-hysteresis", QJsonValue("100")},
    {"adc-zs-postgate-samples", QJsonValue("0")},
    {"adc-zs-pregate-samples", QJsonValue("0")},
    {"adc-zs-threshold", QJsonValue("-29706")},
    {"adc-rtb-mode-enable", QJsonValue("0")},
    {"adc-avg-mode-enable", QJsonValue("0")},
    {"adc-rtb-scans", QJsonValue("4")},
    {"adc-data-inversion", QJsonValue("0")}
};

QHash<QString, QJsonValue> CommandGenerator::smpMap{
    {"smp-type", QJsonValue("Calibration Tuning Mix")},
    {"smp-position", QJsonValue("p1-a1")},
    {"smp-concentration", QJsonValue("50.0")},
    {"smp-injection-volume", QJsonValue("")},
    {"smp-plate-code", QJsonValue("")},
    {"smp-rack-code", QJsonValue("")},
    {"smp-dilution-factor", QJsonValue("")},
    {"smp-plate-position", QJsonValue("")},
    {"smp-balance-type", QJsonValue("")}
};

QHash<QString, QJsonValue> CommandGenerator::usrMap{
    {"usr-role", QJsonValue("Operator")},
    {"usr-name", QJsonValue("service")},
    {"usr-group", QJsonValue("MOBILIon Systems, Inc")}
};

QHash<QString, QJsonValue> CommandGenerator::acqMap{
    {"acq-Ic-model", QJsonValue("Agilent 1290 Infinity II")},
    {"acq-ms-method", QJsonValue("")},
    {"acq-ms-model", QJsonValue("Agilent 6545XT")},
    {"acq-num-frames", QJsonValue("")},
    {"acq-slim_path_length", QJsonValue("")},
    {"acq_software-version", QJsonValue("release-v1.0.27")},
    {"acq-timestamp", QJsonValue("2022-02-10 16:32:33.977127+00:00")},
    {"acq-tune-file", QJsonValue("")},
    {"acq-type", QJsonValue("Sample")},
    {"acq-vendor-metadata", QJsonValue("906b3a6910d84259a810e9d8c35a564f")}
};

QHash<QString, QJsonValue> CommandGenerator::frmMap{
    {"frm-metadata-id",  QJsonValue("1")},
    {"frm-mux-gate",  QJsonValue("0")},
    {"frm-method-state", QJsonValue("")}, //too much
    {"frm-method-name",  QJsonValue("TuneMixTwAmp30")},
    {"frm-timing-intents", QJsonValue("")}, //too much
    {"frm-polarity", QJsonValue("Negative")}
};

QString CommandGenerator::ceProfile = "[{\"number_of_frames\": 1, \"frame_profile\": {\"interval_ms\": 1.0, \"setpoints\": [{\"interval_count\": 1, \"collision_energy_v\": 0.0}]}}, "
                                      "{\"number_of_frames\": 1, \"frame_profile\": {\"interval_ms\": 1.0, \"setpoints\": [{\"interval_count\": 1, \"collision_energy_v\": 30.0}]}}]";

CommandGenerator::CommandGenerator(QObject *parent)
    : QObject{parent}
{

}

QJsonObject CommandGenerator::getCommand(CommandType type, QString fileName, bool maf, int ceVoltage)
{
    QJsonObject command;
    initCommand(command);
    switch (type) {
    case CommandType::Initialization:
        updateInitDigtizerCommand(command);
        break;
    case CommandType::Start_Acquisition:
        updateStartAcqCommand(command, fileName, maf, ceVoltage);
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
    QHash<QString, QJsonValue>::const_iterator adc = adcMap.constBegin();
    while(adc != CommandGenerator::adcMap.constEnd()){
        initDigitizer.insert(adc.key(), adc.value());
        adc++;
    }

    command.insert("data", initDigitizer);
}

void CommandGenerator::updateStartAcqCommand(QJsonObject &command, QString fileName, bool maf, int ceVoltage)
{
    talismanUUID = getUUID();

    QJsonArray target;
    target.append("ACORN_ACQUIRE");
    target.append("ACORN_ARCHIVE");
    target.append("ACORN_STREAM");
    command.insert("target", QJsonValue(target));

    command.insert("command", QJsonValue("START_ACQUISITION"));
    command.insert("talisman", talismanUUID);

    QJsonObject data;

    data.insert("filename", QJsonValue("/acorn/data/" + fileName)); // "/acorn/data/20220228/b.mbi"

    QJsonObject frameObject;
    QHash<QString, QJsonValue>::const_iterator frm = frmMap.constBegin();
    while(frm != frmMap.constEnd()){
        frameObject.insert(frm.key(), frm.value());
        frm++;
    }

    if(DataProcess::isNonDefaultMsCalibrationAvailable()){
        frameObject.insert("cal-ms-traditional", DataProcess::msCalibration);
    }else{
        frameObject.insert("cal-ms-traditional", QString("{\"slope\":%1,\"intercept\":%2,\"mz_residual_terms\":[%3]}").arg(DataProcess::SLOPE_DEFAULT).arg(DataProcess::INTERCEPT_DEFAULT).arg(DataProcess::getResidule()));
    }

    QJsonArray frameArray;
    frameArray.insert(0, QJsonValue(frameObject));

    data.insert("frame", frameArray);

    QJsonObject global;

    QHash<QString, QJsonValue>::const_iterator smp = smpMap.constBegin();
    while(smp != smpMap.constEnd()){
        global.insert(smp.key(), smp.value());
        smp++;
    }

    QHash<QString, QJsonValue>::const_iterator usr = usrMap.constBegin();
    while(usr != usrMap.constEnd()){
        global.insert(usr.key(), usr.value());
        usr++;
    }

    QHash<QString, QJsonValue>::const_iterator adc = adcMap.constBegin();
    while(adc != adcMap.constEnd()){
        global.insert(adc.key(), adc.value());
        adc++;
    }

    QHash<QString, QJsonValue>::const_iterator acq = acqMap.constBegin();
    while(acq != acqMap.constEnd()){
        global.insert(acq.key(), acq.value());
        acq++;
    }

    if(maf){
       ceProfile.replace("30.0", QString::number(ceVoltage));
       global.insert("ce-profile", ceProfile);
    }

    data.insert("global", global);

    command.insert("data", data);
}

void CommandGenerator::updateStopAcqCommand(QJsonObject &command)
{
    QJsonArray target;
    target.append("ACORN_ACQUIRE");
    command.insert("target", QJsonValue(target));
    command.insert("command", QJsonValue("STOP_ACQUISITION"));
    command.insert("talisman", talismanUUID);
}

QString CommandGenerator::getUUID()
{
    return QUuid::createUuid().toString().remove('-').remove('{').remove('}');
}

QString CommandGenerator::currentTalismanUUID() const
{
    return talismanUUID;
}

unsigned int CommandGenerator::lastUsedSequency() const
{
    return sequence - 1;
}

void CommandGenerator::updateInfo(QString key, QString value)
{
    if(adcMap.contains(key))
        adcMap[key] = QJsonValue(value);

    if(smpMap.contains(key))
        smpMap[key] = QJsonValue(value);

    if(usrMap.contains(key))
        usrMap[key] = QJsonValue(value);

    if(acqMap.contains(key))
        acqMap[key] = QJsonValue(value);

    if(frmMap.contains(key))
        frmMap[key] = QJsonValue(value);
}

void CommandGenerator::updateOffsetForInversion()
{
    qDebug() << "update offset";
    double offset = adcMap["adc-offset"].toString().toDouble();
    qDebug() << "offset: " << offset;
    if(adcMap["adc-data-inversion"].toString().toInt() == 0){
        if(offset < 0)
            adcMap["adc-offset"] = QJsonValue(QString::number(offset * (-1)));
    }else{
        if(offset > 0)
            adcMap["adc-offset"] = QJsonValue(QString::number(offset * (-1)));
    }
    qDebug() << "new offset: " << adcMap["adc-offset"];
}
