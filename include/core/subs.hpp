#include "core/parser.hpp"

#include <mosquitto.h>
#include <string>
#include <vector>

#include <chrono>

class MQTTSubscriber {
public:
    MQTTSubscriber(const std::string& broker, int port, const std::vector<std::string>& topic, std::vector<std::shared_ptr<Parser>>& queueConsumer);
    ~MQTTSubscriber();
    void init();
    void subscribe();

private:
    std::string broker_;
    int port_;
    std::vector<std::string> topic_;
    struct mosquitto* mosq_;
    std::vector<std::shared_ptr<Parser>>& queueConsumer;
    int topic_num;

    // debug0
    std::chrono::_V2::steady_clock::time_point tik;
    std::chrono::_V2::steady_clock::time_point tak;
    // debug-1

    static void onMessageWrapper(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* message);
    void onMessage(const struct mosquitto_message* message);
};