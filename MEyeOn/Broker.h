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
#include "commandGenerator.h"

namespace Kafka {
class Broker;
class AcornDeliveryReportCb;
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

    ~Broker();

    void initDigitizer();
    void startAcquire();
    void stopAcquire();

private:
    bool startedAcquire = false;
    CommandGenerator commandGen;
    std::string _brokers;
    RdKafka::Conf* _conf{nullptr};
    RdKafka::Producer* _producer;

    bool _config_error;
    std::string _recent_config_error;

    void config();

    void SetConfigError(bool err, std::string err_msg);
};
