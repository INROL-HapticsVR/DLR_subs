#include "core/subs.hpp"
#include <iostream>
#include <algorithm>

MQTTSubscriber::MQTTSubscriber(const std::string& broker, int port, const std::vector<std::string>& topic, std::vector<std::shared_ptr<Parser>>& queueConsumer)
    : broker_(broker), port_(port), topic_(topic), mosq_(nullptr), queueConsumer(queueConsumer) {
    topic_num = topic_.size();
    init();
}

MQTTSubscriber::~MQTTSubscriber() {
    if (mosq_) {
        mosquitto_destroy(mosq_); // Mosquitto 클라이언트 제거
        mosq_ = nullptr;
    }
    mosquitto_lib_cleanup(); // Mosquitto 라이브러리 정리
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

    // dg0
    tik = std::chrono::steady_clock::now();
}

void MQTTSubscriber::subscribe() {
    mosquitto_message_callback_set(mosq_, MQTTSubscriber::onMessageWrapper);

    // 모든 토픽에 대해 구독
    for (const auto& topic : topic_) {
        if (mosquitto_subscribe(mosq_, nullptr, topic.c_str(), 0) != MOSQ_ERR_SUCCESS) {
            std::cerr << "Failed to subscribe to topic: " << topic << std::endl;
            return;
        }
        std::cout << "Subscribed to topic: " << topic << std::endl;
    }

    while(1){
        mosquitto_loop(mosq_,5,1);
    }
    // MQTT 메시지 루프 시작
    //int ret = mosquitto_loop_forever(mosq_, -1, 1);
    //if (ret != MOSQ_ERR_SUCCESS) {
    //    std::cerr << "Mosquitto loop exited with error: " << ret << std::endl;
    //}
}


void MQTTSubscriber::onMessageWrapper(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* message) {
    if (!userdata) return;
    auto* self = static_cast<MQTTSubscriber*>(userdata);

    self->onMessage(message);
}

void MQTTSubscriber::onMessage(const struct mosquitto_message* message) {
    if (message->payloadlen > 0) {
        std::string topic(message->topic); // 수신된 메시지의 토픽
        std::string payload(static_cast<char*>(message->payload), message->payloadlen);

        // 토픽 필터링
        auto it = std::find(topic_.begin(), topic_.end(), topic);
        if (it != topic_.end()) {
            int index = std::distance(topic_.begin(), it); // 매칭된 토픽의 인덱스 계산
            queueConsumer[index]->push(payload);   // 매칭된 Parser에 메시지 전달
        } else {
            std::cerr << "[Warning] Received message for unregistered topic: " << topic << std::endl;
        }

        // FPS 계산
        tak = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(tak - tik);
        std::cout << "[Debug] FPS: " << 1000 / duration.count() << " FPS" << std::endl;
        tik = tak;
    }
}
