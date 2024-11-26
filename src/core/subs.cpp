#include "core/subs.hpp"
#include <iostream>

MQTTSubscriber::MQTTSubscriber(const std::string& broker, int port, const std::string& topic, Parser& queueConsumer)
    : broker_(broker), port_(port), topic_(topic), mosq_(nullptr), queueConsumer(queueConsumer) {
    init();
}

MQTTSubscriber::~MQTTSubscriber() {
    if (mosq_) {
        mosquitto_destroy(mosq_); // Mosquitto 클라이언트 제거
        mosq_ = nullptr;
    }
    mosquitto_lib_cleanup(); // Mosquitto 라이브러리 정리
}

void MQTTSubscriber::subscribe() {
    mosquitto_message_callback_set(mosq_, MQTTSubscriber::onMessageWrapper);

    if (mosquitto_subscribe(mosq_, nullptr, topic_.c_str(), 0) != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to subscribe to topic: " << topic_ << std::endl;
        return;
    }

    std::cout << "Subscribed to topic: " << topic_ << std::endl;

    int ret = mosquitto_loop_forever(mosq_, -1, 1);
    if (ret != MOSQ_ERR_SUCCESS) {
        std::cerr << "Mosquitto loop exited with error: " << ret << std::endl;
    }
}

void MQTTSubscriber::init() {
    mosquitto_lib_init(); // Mosquitto 라이브러리 초기화
    mosq_ = mosquitto_new(nullptr, true, this);
    if (!mosq_) {
        std::cerr << "Failed to create Mosquitto client." << std::endl;
        mosquitto_lib_cleanup();
        return;
    }

    if (mosquitto_connect(mosq_, broker_.c_str(), port_, 60) != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect to broker." << std::endl;
        mosquitto_destroy(mosq_);
        mosq_ = nullptr;
        mosquitto_lib_cleanup();
        return;
    }
}

void MQTTSubscriber::onMessageWrapper(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* message) {
    if (!userdata) return;
    auto* self = static_cast<MQTTSubscriber*>(userdata);
    self->onMessage(message);
}

void MQTTSubscriber::onMessage(const struct mosquitto_message* message) {
    if (message->payloadlen > 0) {
        std::string topic(message->topic);
        std::string payload(static_cast<char*>(message->payload), message->payloadlen);
        queueConsumer.push(topic, payload); // 메시지를 큐에 추가
    }
}
