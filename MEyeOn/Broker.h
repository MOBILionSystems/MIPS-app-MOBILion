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
#include <QElapsedTimer>

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

enum AckNack{Ack = 0, Nack, Empty, Other, TimeOut};

class Broker : public QObject {
    Q_OBJECT
public:
    explicit Broker(QString ipaddress, QObject* parent = nullptr);

    ~Broker();

    void initDigitizer();
    void startAcquire(QString fileName);
    void stopAcquire();
    bool isAcqiring();
    AckNack getAck(QString sequence, QString service, QString command);


signals:
    void acqAckNack(AckNack response);
    void configureAckNack(AckNack response);

private:
    QElapsedTimer ackTimer;
    bool ackTimerStarted = false;
    QString _ipaddress;
    bool startedAcquire = false;
    CommandGenerator commandGen;
    std::string _brokers;
    RdKafka::Conf* _producerConf{nullptr};
    RdKafka::Conf* _consumerConf{nullptr};
    RdKafka::Producer* _cmdProducer;
    RdKafka::KafkaConsumer* _statusConsumer;
    std::string groupID{};

    bool _config_error;
    std::string _recent_config_error;

    void config();

    void SetConfigError(bool err, std::string err_msg);
    std::string GenerateRandomHexString(const unsigned int len);

    void waitAcqAck(unsigned int timeOutMs = 500);
    void waitInitAck(unsigned int timeOutMs = 10000);
};
