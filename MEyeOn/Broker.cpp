#include "Broker.h"
#include <QJsonDocument>
#include <QThread>
#include <QJsonDocument>
#include <QTimer>

Broker::Broker(QString ipaddress, QObject* parent) :
    QObject(parent)
{
    _ipaddress = ipaddress;
    SetConfigError(false, std::string());
    config();

    if (!_config_error) {
        std::string err_msg;

        _cmdProducer = RdKafka::Producer::create(_producerConf, err_msg);
        if (!_cmdProducer) {
            SetConfigError(true, err_msg);
            return;
        }

        _statusConsumer = RdKafka::KafkaConsumer::create(_consumerConf, err_msg);
        if(!_statusConsumer){
            SetConfigError(true, err_msg);
            return;
        }

        std::vector<std::string> topics = {"ACORN-STATUS"};
        RdKafka::ErrorCode err = _statusConsumer->subscribe(topics);
        if (err) {
            SetConfigError(true, "Cannot subscribe consumer to topic: " + RdKafka::err2str(err));
        }
        // Poll to get in sync with current state
        _statusConsumer->poll(0);

    }
}

Broker::~Broker()
{
    if(startedAcquire)
        stopAcquire();
    delete _cmdProducer;
    delete _producerConf;
    delete _statusConsumer;
    delete _consumerConf;
}

void Broker::initDigitizer()
{
    //qDebug() << "initDigitizer";
    QString dest = "ACORN-CMD";
    QJsonObject commandObject = commandGen.getCommand(CommandType::Initialization);
    std::string payload = QJsonDocument(commandObject).toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    char* payload_c = const_cast<char*>(payload.c_str());
    RdKafka::ErrorCode resp = _cmdProducer->produce(dest.toStdString(), RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
                                                    payload_c, payload.size(), NULL, 0, 0, NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
        qWarning() << "% Produce failed [" << dest << "]:" <<
                      QString::fromStdString(RdKafka::err2str(resp)) << "|" << resp;
    }else{
        waitInitAck();
    }
}

void Broker::startAcquire(QString fileName)
{
    if(startedAcquire){
        stopAcquire();
        QThread::sleep(2);
    }

    QString dest = "ACORN-CMD";
    QJsonObject commandObject = commandGen.getCommand(CommandType::Start_Acquisition, fileName);
    std::string payload = QJsonDocument(commandObject).toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    char* payload_c = const_cast<char*>(payload.c_str());
    RdKafka::ErrorCode resp = _cmdProducer->produce(dest.toStdString(), RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
                                                    payload_c, payload.size(), NULL, 0, 0, NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
        qWarning() << "% Produce failed [" << dest << "]:" <<
                      QString::fromStdString(RdKafka::err2str(resp)) << "|" << resp;
    }else{
        startedAcquire = true;
        waitAcqAck();
    }
}

void Broker::stopAcquire()
{
    QString dest = "ACORN-CMD";
    QJsonObject commandObject = commandGen.getCommand(CommandType::Stop_Acquisition);
    std::string payload = QJsonDocument(commandObject).toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    char* payload_c = const_cast<char*>(payload.c_str());
    RdKafka::ErrorCode resp = _cmdProducer->produce(dest.toStdString(), RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
                                                    payload_c, payload.size(), NULL, 0, 0, NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
        qWarning() << "% Produce failed [" << dest << "]:" <<
                      QString::fromStdString(RdKafka::err2str(resp)) << "|" << resp;
    }else{
        startedAcquire = false;
    }
}

bool Broker::isAcqiring()
{
    return startedAcquire;
}

void Broker::waitAcqAck(unsigned int timeOutMs){
    if(!ackTimerStarted){
        ackTimer.start();
        ackTimerStarted = true;
    }

    int remainingMs = timeOutMs - ackTimer.elapsed();
    if(remainingMs > 0){
        AckNack response = getAck(QString::number(commandGen.lastUsedSequency()), "ACORN_ACQUIRE", "START_ACQUISITION");
        if(response == AckNack::Ack || response == AckNack::Nack){
            // qDebug() << "Ack or Nack in " << timeOutMs - remainingMs << " ms.";
            ackTimerStarted = false;
            emit acqAckNack(response);
        }else if(response == AckNack::Empty){
            QTimer::singleShot(10, [=](){waitAcqAck();});
        }else{
            // qDebug() << "Other";
            waitAcqAck(timeOutMs);
        }
    }else{
        ackTimerStarted = false;
        emit acqAckNack(AckNack::TimeOut);
    }
}

void Broker::waitInitAck(unsigned int timeOutMs)
{
    //qDebug() << "waitInitAck";
    if(!ackTimerStarted){
        ackTimer.start();
        ackTimerStarted = true;
    }

    int remainingMs = timeOutMs - ackTimer.elapsed();
    if(remainingMs > 0){
        AckNack response = getAck(QString::number(commandGen.lastUsedSequency()), "ACORN_ACQUIRE", "CONFIGURE_DIGITIZER");
        if(response == AckNack::Ack || response == AckNack::Nack){
            //qDebug() << "Ack or Nack in " << timeOutMs - remainingMs << " ms.";
            ackTimerStarted = false;
            emit configureAckNack(response);
        }else if(response == AckNack::Empty){
            QTimer::singleShot(100, [=](){waitInitAck();});
        }else{
            //qDebug() << "Other";
            waitAcqAck(timeOutMs);
        }
    }else{
        ackTimerStarted = false;
        //qDebug() << "Init timeout.";
        emit configureAckNack(AckNack::TimeOut);
    }
}

AckNack Broker::getAck(QString sequence, QString service, QString command)
{
    //    qDebug() << "--------------------";
    RdKafka::Message *msg = _statusConsumer->consume(0);

    if (!msg || msg->err() != RdKafka::ERR_NO_ERROR) {
        delete msg;
        //qDebug() << "Empty msg";
        return AckNack::Empty;
    }
    QString payload = QString::fromUtf8(static_cast<char*>(msg->payload()));
    //    qDebug() << "payload: " << payload;
    delete msg;
    QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
    if(doc.isNull()) return AckNack::Other;
    //    qDebug() << doc;

    QJsonValue mod = doc["module"];
    if(mod == QJsonValue::Undefined){
        //qDebug() << "1";
        return AckNack::Other;
    }

    QJsonValue ser = doc["service"];
    if(ser == QJsonValue::Undefined) {
        //qDebug() << "1";
        return AckNack::Other;
    }

    QJsonValue com = doc["info"]["echo"]["command"];
    if(com == QJsonValue::Undefined) {
        //qDebug() << "2";
        return AckNack::Other;
    }

    QJsonValue seq = doc["info"]["echo"]["sequence"];
    if(seq == QJsonValue::Undefined) {
        //qDebug() << "3";
        return AckNack::Other;
    }

    if(ser.toString() != service || com.toString() != command || seq.toString() != sequence){
        //        qDebug() << ser.toString() << ", " << service;
        //        qDebug() << com.toString() << ", " << command;
        //        qDebug() << seq.toString() << ", " << sequence;
        //        qDebug() << "4";
        return AckNack::Other;
    }

    if(mod.toString() == "ack"){
        //qDebug() << "ACK";
        return AckNack::Ack;
    }else if(mod.toString() == "nack"){
        //qDebug() << "NACK";
        return AckNack::Nack;
    }

    return AckNack::Other;
}


void Broker::config()
{
    _producerConf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

    std::string err_msg;
    AcornDeliveryReportCb ac_dr_cb;

    if (_producerConf->set("bootstrap.servers", _ipaddress.toStdString(), err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }

    if (_producerConf->set("socket.timeout.ms", "5000", err_msg) != RdKafka::Conf::CONF_OK){
        SetConfigError(true, err_msg);
        return;
    }

    if (_producerConf->set("message.timeout.ms", "5000", err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }

    if (_producerConf->set("dr_cb", &ac_dr_cb, err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }

    _consumerConf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    RdKafka::Conf* consumer_topic_config = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    _consumerConf->set("enable.partition.eof", "true", err_msg);
    if (_consumerConf->set("group.id", "ACORN_ACQUIRE-" + GenerateRandomHexString(32), err_msg) !=
            RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;  // Give up here and fail the configuration
    }

    if (_consumerConf->set("bootstrap.servers", _ipaddress.toStdString(), err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
    }

    if (_consumerConf->set("auto.offset.reset", "latest", err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }

    if (_consumerConf->set("auto.commit.interval.ms", "500", err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }

    if (_consumerConf->set("default_topic_conf", consumer_topic_config, err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }

    if (_consumerConf->set("session.timeout.ms", "60000", err_msg) != RdKafka::Conf::CONF_OK){
        SetConfigError(true, err_msg);
        return;
    }

    if (_consumerConf->set("enable.auto.commit", "True", err_msg) != RdKafka::Conf::CONF_OK){
        SetConfigError(true, err_msg);
        return;
    }
}

void Broker::SetConfigError(bool err, std::string err_msg)
{
    _config_error = err;
    _recent_config_error = err_msg;
    if(err)
        qWarning() << "Error in config: " << QString::fromStdString(err_msg);
}

std::string Broker::GenerateRandomHexString(const unsigned int len)
{
    std::stringstream ss;
    for (unsigned int i = 0; i < len; i++) {
        QString rc = CommandGenerator::getUUID();
        std::stringstream hexstream;
        hexstream << std::hex << std::setw(2) << std::setfill('0') << rc.toStdString();
        auto hex = hexstream.str();
        ss << hex;
    }
    return ss.str();
}

