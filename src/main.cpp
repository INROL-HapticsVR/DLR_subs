#include "core/parser.hpp"
#include "core/subs.hpp"
#include <thread>
#include <vector>
#include <memory>

int main() {
    // MQTT 브로커 정보 및 토픽 정의
    std::string broker_address = "147.46.149.20";
    int port = 1883;
    std::vector<std::string> topics = {"topic/camera0", "topic/camera1", "topic/camera2"};
    // std::vector<std::string> topics = {"topic/camera0", "topic/camera1"};

    // Parser와 MQTTSubscriber 객체를 동적으로 관리
    std::vector<std::shared_ptr<Parser>> queueConsumers;

    for (int i = 0; i < topics.size(); i++) {
        // 각 토픽에 대해 Parser를 생성, topic 인덱스를 전달
        auto parser = std::make_shared<Parser>(i); // 인자로 토픽의 인덱스 전달
        queueConsumers.push_back(parser);          // 소유권 공유
    }

    // MQTTSubscriber 생성
    auto subscriber = std::make_unique<MQTTSubscriber>(broker_address, port, topics, queueConsumers);

    // 구독 쓰레드 생성
    std::thread subscriberThread([&subscriber]() {
        subscriber->subscribe();
    });

    // 소비 쓰레드 생성
    std::vector<std::thread> consumerThreads;
    for (size_t i = 0; i < queueConsumers.size(); i++) {
        consumerThreads.emplace_back([queueConsumer = queueConsumers[i]]() {
            queueConsumer->consume();
        });
    }

    // 쓰레드 합류
    subscriberThread.join();
    for (auto& thread : consumerThreads) {
        thread.join();
    }

    return 0;
}
