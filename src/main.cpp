#include "core/parser.hpp"
#include "core/subs.hpp"
#include <thread>
#include <vector>
#include <memory>

int main() {
    // MQTT 브로커 정보 및 토픽 정의
    std::string broker_address = "192.168.0.7";
    int port = 1883;
    // std::vector<std::string> topics = {"topic/camera2", "topic/camera1", "topic/camera0"};
    std::vector<std::string> topics = {"topic/camera0"};
    // Parser와 MQTTSubscriber 객체를 동적으로 관리
    std::vector<std::unique_ptr<Parser>> queueConsumers;
    std::vector<std::unique_ptr<MQTTSubscriber>> subscribers;

    for (int i = 0; i < topics.size(); i++) {
        // 각 토픽에 대해 Parser를 생성, topic 인덱스를 전달
        auto parser = std::make_unique<Parser>(i); // 인자로 토픽의 인덱스 전달
        queueConsumers.push_back(std::move(parser));          // 소유권 공유
        auto subscriber = std::make_unique<MQTTSubscriber>(broker_address, port, topics[i], *queueConsumers.back()); 
        subscribers.push_back(std::move(subscriber));
    }


    // 쓰레드 배열
    std::vector<std::thread> subscriberThreads;
    std::vector<std::thread> consumerThreads;

    // 구독 쓰레드 생성
    for (size_t i = 0; i < subscribers.size(); ++i) {
        subscriberThreads.emplace_back([&subscribers, i]() {
            subscribers[i]->subscribe();
        });
    }
    
    // 소비 쓰레드 생성
    for (size_t i = 0; i < queueConsumers.size(); ++i) {
        consumerThreads.emplace_back([&queueConsumers, i]() {
            queueConsumers[i]->consume();
        });
    }

    // 모든 쓰레드 합류
    for (auto& thread : subscriberThreads) {
        thread.join();
    }
    for (auto& thread : consumerThreads) {
        thread.join();
    }

    return 0;
}
