#ifndef PARSER_HPP
#define PARSER_HPP

#include <deque> // std::deque 사용
#include <mutex>
#include <string>
#include <condition_variable>
#include <iostream>
#include <cstdint>
#include <array>

// # define BUFFER_NUM 2
// # define USE_RENDERING
# define WIDTH 1200
# define HEIGHT 680

struct image_shared {
    // int width = WIDTH;
    // int height = HEIGHT;
    std::array<uint8_t, WIDTH * HEIGHT> r;   // RGB 데이터
    std::array<uint8_t, WIDTH * HEIGHT> g;
    std::array<uint8_t, WIDTH * HEIGHT> b;
    std::array<uint8_t, WIDTH * HEIGHT> depth;    // Depth 데이터
};

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
    Parser(int topic);
    ~Parser();

    void push(const std::string& payload);
    std::string pop();

    void consume();
    void parsing(std::string payload);
    void displayRGBImage();
    void displayDepthImage();

    std::string topic_idx;
    std::string payload;

    // shared memory
    std::string shm_name;
    int shm_fd;
    size_t shm_size;
    uint8_t* shm_ptr;

    int activeBuffer;               // 활성 버퍼 인덱스 (0 또는 1)
    int64_t time;
    image_rgb_t rgb;
    image_d_t depth;

private:
    std::string latestPayload;            // 가장 최근의 메시지
    std::mutex dataMutex;                 // 데이터 보호를 위한 mutex
    std::condition_variable dataCondition;
};

#endif // PARSER_HPP
