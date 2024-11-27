#ifndef PARSER_HPP
#define PARSER_HPP

#include <deque> // std::deque 사용
#include <mutex>
#include <string>
#include <condition_variable>
#include <iostream>

# define BUFFER_NUM 2
# define WIDTH 1280
# define HEIGHT 720

struct image_rgb_t {
    int width = WIDTH;
    int height = HEIGHT;
    std::array<uint8_t, WIDTH * HEIGHT> r, g, b;
};

struct image_d_t {
    int width = WIDTH;
    int height = HEIGHT;
    std::array<uint8_t, WIDTH * HEIGHT> d;
};

class Parser {
public:
    void push(const std::string& topic, const std::string& payload);
    std::pair<std::string, std::string> pop();

    void consume();
    void parsing(std::string payload);
    void displayRGBImage();
    void displayDepthImage();

    std::string topic;
    std::string payload;

    int64_t time;
    image_rgb_t rgb;
    image_d_t depth;

private:
    static constexpr size_t MAX_QUEUE_SIZE = BUFFER_NUM;
    std::deque<std::pair<std::string, std::string>> messageQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
};

#endif // PARSER_HPP
