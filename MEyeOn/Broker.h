#pragma once

#include <rdkafkacpp.h>
#include <QObject>
#include <QJsonDocument>
#include <QDebug>
#include <sstream>
#include <random>
#include <string>
#include <iomanip>
#include <iostream>

namespace Kafka {
class Broker;
class AcornDeliveryReportCb;
class AcornEventCb;
}


class AcornDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
    /**
     * @brief Delivery report callback function called by Kafka when a sent
     * message is produced successfully.
     *
     * @param message The message associated with the state of the produced
     * message, conveying success or failure details.
     * @note Breaking style with this method to match the
     * RdKafka::DeliveryReportCb interface.
     */
    void dr_cb(RdKafka::Message& message){  // NOLINT: Kafka library expects non-const ref.
        if (message.err()) {
            std::cerr << "% Message delivery failed: " << message.errstr()
                      << std::endl;
        }else{
            std::cerr << "% Message delivered to topic " << message.topic_name()
                      << " [" << message.partition() << "] at offset "
                      << message.offset() << std::endl;
        }
    }
};

class Broker : public QObject {
    Q_OBJECT
public:
    explicit Broker(QObject* parent = nullptr);

    void Read();
    void Write(std::string dest, std::string payload);

    std::string _status_topic;
    /// @brief Topic to which log messages are produced.
    std::string _log_topic;
    /// @brief Topic from which command messages are consumed.
    std::vector<std::string> _command_topic;

private:
    std::string _brokers;
    std::string _topic;
    std::string _errstr = "";
    RdKafka::Conf* _conf{nullptr};
    RdKafka::Producer* _producer;

    bool _config_error;
    std::string _recent_config_error;

    void config();

    void SetConfigError(bool err, std::string err_msg);
};
