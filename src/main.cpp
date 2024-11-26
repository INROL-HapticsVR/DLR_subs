// #include "Test/Test1.hpp"
// #include "Test2.hpp"
// #include "Test3.hpp"

#include "core/parser.hpp"
#include "core/subs.hpp"
#include <thread>

int main() {
    // 메시지 큐와 소비자 생성
    Parser queueConsumer;

    // MQTTSubscriber 생성
    MQTTSubscriber subscriber("147.46.149.20", 1883, "topic/camera0", queueConsumer);

    // 메시지 구독 쓰레드
    std::thread subscriberThread([&subscriber]() {
        subscriber.subscribe();
    });

    // 메시지 소비 쓰레드
    std::thread consumerThread([&queueConsumer]() {
        queueConsumer.consume();
    });

    // 쓰레드 합류
    subscriberThread.join();
    consumerThread.join();

    return 0;
}