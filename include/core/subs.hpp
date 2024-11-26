#include "core/parser.hpp"

#include <mosquitto.h>
#include <string>

class MQTTSubscriber {
public:
    MQTTSubscriber(const std::string& broker, int port, const std::string& topic, Parser& queueConsumer);
    ~MQTTSubscriber();

    void subscribe();

private:
    std::string broker_;
    int port_;
    std::string topic_;
    struct mosquitto* mosq_;
    Parser& queueConsumer;

    void init();
    static void onMessageWrapper(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* message);
    void onMessage(const struct mosquitto_message* message);
};