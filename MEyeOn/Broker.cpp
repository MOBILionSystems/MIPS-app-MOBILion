#include "Broker.h"
#include <QJsonDocument>

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
}

void Broker::initDigitizer()
{
    QString dest = "ACORN-CMD";
    QJsonObject commandObject = commandGen.getCommand(CommandType::Initialization);
    std::string payload = QJsonDocument(commandObject).toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    char* payload_c = const_cast<char*>(payload.c_str());
    RdKafka::ErrorCode resp = _producer->produce(dest.toStdString(), RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
                                                 payload_c, payload.size(), NULL, 0, 0, NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
        qWarning() << "% Produce failed [" << dest << "]:" <<
                      QString::fromStdString(RdKafka::err2str(resp)) << "|" << resp;
    }
}

void Broker::startAcquire()
{
    QString dest = "ACORN-CMD";
    QJsonObject commandObject = commandGen.getCommand(CommandType::Start_Acquisition);
    std::string payload = QJsonDocument(commandObject).toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    char* payload_c = const_cast<char*>(payload.c_str());
    RdKafka::ErrorCode resp = _producer->produce(dest.toStdString(), RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
                                                 payload_c, payload.size(), NULL, 0, 0, NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
        qWarning() << "% Produce failed [" << dest << "]:" <<
                      QString::fromStdString(RdKafka::err2str(resp)) << "|" << resp;
    }
}

void Broker::stopAcquire()
{
    QString dest = "ACORN-CMD";
    QJsonObject commandObject = commandGen.getCommand(CommandType::Stop_Acquisition);
    std::string payload = QJsonDocument(commandObject).toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    char* payload_c = const_cast<char*>(payload.c_str());
    RdKafka::ErrorCode resp = _producer->produce(dest.toStdString(), RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
                                                 payload_c, payload.size(), NULL, 0, 0, NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
        qWarning() << "% Produce failed [" << dest << "]:" <<
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

