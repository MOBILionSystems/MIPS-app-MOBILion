#include "Broker.h"

Broker::Broker(QObject* parent) :
    QObject(parent)
{
    SetConfigError(false, std::string());
    config();
    // If no configuration error so far, initialize consumer / producer.
    if (!_config_error) {
        std::string err_msg;
//        _consumer = RdKafka::KafkaConsumer::create(_consumer_conf, err_msg);
//        if (!_consumer) {
//            SetConfigError(true, err_msg);
//            return;
//        }
        _producer = RdKafka::Producer::create(_producer_conf, err_msg);
        if (!_producer) {
            SetConfigError(true, err_msg);
            return;
        }



        // Subscribe consumer to Command Topic
//        _command_topic = { "ACORN-LOG" };
//        RdKafka::ErrorCode err = _consumer->subscribe(_command_topic);
//        if (err) {
//            SetConfigError(true, "Cannot subscribe consumer to topic: " + RdKafka::err2str(err));
//        }
//        _consumer->poll(0);
    }
    _status_topic = "ACORN-STATUS";
    _log_topic = "ACORN-LOG";
}

void Broker::Read()
{
}

void Broker::Write(std::string dest, std::string payload)
{
    qDebug() << "dest: " << QString::fromStdString(dest);
    qDebug() << "payload: " << QString::fromStdString(payload);
    char* payload_c = new char[payload.length() + 1];
    std::strcpy(payload_c, payload.c_str());
    RdKafka::ErrorCode resp = _producer->produce(dest, RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY, 
        payload_c, payload.size(), NULL, 0, 0, NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
        qWarning() << "% Produce failed [" << QString::fromStdString(dest) << "]:" << 
            QString::fromStdString(RdKafka::err2str(resp)) << "|" << resp;
    }
}


void Broker::config()
{
    _producer_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
//    _consumer_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

//    RdKafka::Conf* producer_topic_config = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
//    RdKafka::Conf* consumer_topic_config = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

    std::string err_msg;
    AcornDeliveryReportCb* ac_dr_cb = new AcornDeliveryReportCb();
    //AcornEventCb* ac_event_cb = new AcornEventCb();

    /* Consumer Config */   

//    _consumer_conf->set("enable.partition.eof", "true", err_msg);
    
    
//    if (_consumer_conf->set("group.id", "ACORN_ACQUIRE-" + GenerateRandomHexString(32), err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }
    

//    if (_consumer_conf->set("bootstrap.servers", "localhost:9092", err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//    }

//    if (_consumer_conf->set("auto.offset.reset", "latest", err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }

//    if (_consumer_conf->set("auto.commit.interval.ms", "500", err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }

//    if (_consumer_conf->set("event_cb", ac_event_cb, err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }

//    if (_consumer_conf->set("default_topic_conf", consumer_topic_config, err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }

    

    /*
     Producer Config
 */

    
    if (_producer_conf->set("bootstrap.servers", "192.168.1.212:9092", err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }
    

//    if (_producer_conf->set("socket.timeout.ms", "5000", err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }

//    if (_producer_conf->set("message.timeout.ms", "5000", err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }

//    if (_producer_conf->set("event_cb", ac_event_cb, err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }

    if (_producer_conf->set("dr_cb", ac_dr_cb, err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }

//    if (_producer_conf->set("default_topic_conf", producer_topic_config, err_msg) != RdKafka::Conf::CONF_OK) {
//        SetConfigError(true, err_msg);
//        return;
//    }

}

unsigned int Broker::GenerateRandomChar()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    return dis(gen);
}

std::string Broker::GenerateRandomHexString(const unsigned int len)
{
    std::stringstream ss;
    for (auto i = 0; i < len; i++) {
        const auto rc = GenerateRandomChar();
        std::stringstream hexstream;
        hexstream << std::hex << std::setw(2) << std::setfill('0') << rc;
        auto hex = hexstream.str();
        ss << hex;
    }
    return ss.str();
}

void Broker::SetConfigError(bool err, std::string err_msg)
{
    _config_error = err;
    _recent_config_error = err_msg;
    qWarning() << "Error in config: " << QString::fromStdString(err_msg);
}

