#include "Broker.h"

Broker::Broker(QObject* parent) :
    QObject(parent)
{
    SetConfigError(false, std::string());
    config();

    if (!_config_error) {
        std::string err_msg;

        _producer = RdKafka::Producer::create(_conf, err_msg);
        if (!_producer) {
            SetConfigError(true, err_msg);
            return;
        }
    }
    _status_topic = "ACORN-STATUS";
    _log_topic = "ACORN-LOG";
}

void Broker::Read()
{
}

void Broker::Write(std::string dest, std::string payload)
{
    char* payload_c = const_cast<char*>(payload.c_str());
    RdKafka::ErrorCode resp = _producer->produce(dest, RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
                                                 payload_c, payload.size(), NULL, 0, 0, NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
        qWarning() << "% Produce failed [" << QString::fromStdString(dest) << "]:" <<
                      QString::fromStdString(RdKafka::err2str(resp)) << "|" << resp;
    }
}


void Broker::config()
{
    _conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

    std::string err_msg;
    AcornDeliveryReportCb ac_dr_cb;

    if (_conf->set("bootstrap.servers", "192.168.1.212:9092", err_msg) != RdKafka::Conf::CONF_OK) {
        SetConfigError(true, err_msg);
        return;
    }

    if (_conf->set("dr_cb", &ac_dr_cb, err_msg) != RdKafka::Conf::CONF_OK) {
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

