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
	void dr_cb(RdKafka::Message& message);  // NOLINT: Kafka library expects non-const ref.
};

class AcornEventCb : public RdKafka::EventCb {
public:
	/**
	 * @brief Event callback function called by Kafka to indicate various events
	 * within the Kafka Broker system.
	 *
	 * @param event The event associated with the provenance of the function call
	 * (e.g. stats, logging, errors).
	 * @note Breaking style with this method to match the RdKafka::EventCb
	 * interface.
	 */
	void event_cb(RdKafka::Event& event);  // NOLINT: Kafka library expects non-const ref.
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
	RdKafka::Conf* _producer_conf{nullptr};
	RdKafka::Conf* _consumer_conf{nullptr};
	RdKafka::Producer* _producer;
	RdKafka::KafkaConsumer* _consumer;

	bool _config_error;
	std::string _recent_config_error;

	void config();

	unsigned int GenerateRandomChar();
	std::string GenerateRandomHexString(const unsigned int len);
	void SetConfigError(bool err, std::string err_msg);
};
