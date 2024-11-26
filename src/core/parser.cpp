#include "core/parser.hpp"

#include <stdexcept>
#include <cstring>
#include <opencv2/opencv.hpp> // OpenCV의 모든 기본 모듈 포함
#include <opencv2/highgui.hpp> // GUI 창을 위한 모듈
#include <opencv2/imgproc.hpp> // 이미지 처리 모듈

void Parser::push(const std::string& topic, const std::string& payload) {
    std::lock_guard<std::mutex> lock(queueMutex);

    // 큐가 가득 차면 가장 오래된 데이터 제거
    if (messageQueue.size() >= MAX_QUEUE_SIZE) {
        messageQueue.pop_front();
    }

    messageQueue.emplace_back(topic, payload);
    queueCondition.notify_one(); // 대기 중인 쓰레드에 알림
}

std::pair<std::string, std::string> Parser::pop() {
    std::unique_lock<std::mutex> lock(queueMutex);
    queueCondition.wait(lock, [this]() { return !messageQueue.empty(); }); // 큐가 비어있으면 대기
    auto message = messageQueue.front();
    messageQueue.pop_front();
    return message;
}

void Parser::consume() {
    // debug-
    int fps;

    while (true) {
        auto message = pop(); // 큐에서 메시지 가져오기
        const auto& topic = message.first;
        const auto& payload = message.second;

        parsing(payload);
        displayRGBImage();
        displayDepthImage();

        //debug
    }
}

void Parser::parsing(std::string payload) {
    if (payload.size() != 8 + 4 * WIDTH * HEIGHT) {
        throw std::runtime_error("Invalid payload size.");
    }

    // 데이터를 바이너리에서 구조체로 변환
    std::memcpy(&time, &payload[0], sizeof(int64_t)); // 첫 8바이트를 int64_t로 변환

    // 하림씨가 BGR로 준다도르
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        rgb.r[i] = static_cast<uint8_t>(payload[8 + 4 * i + 2]);
        rgb.g[i] = static_cast<uint8_t>(payload[8 + 4 * i + 1]);
        rgb.b[i] = static_cast<uint8_t>(payload[8 + 4 * i + 0]);
        depth.d[i] = static_cast<uint8_t>(payload[8 + 4 * i + 3]);
    }
}

void Parser::displayRGBImage() {
    // OpenCV Mat 생성 (3채널 RGB 이미지)
    cv::Mat image(HEIGHT, WIDTH, CV_8UC3);

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            int idx = i * WIDTH + j;
            image.at<cv::Vec3b>(i, j) = cv::Vec3b(rgb.b[idx], rgb.g[idx], rgb.r[idx]);
        }
    }

    // 이미지 표시
    cv::imshow("RGB Image", image);
    cv::waitKey(1); // 1ms 대기 (실시간 처리)
}

void Parser::displayDepthImage() {
    // OpenCV Mat 생성 (1채널 그레이스케일 이미지)
    cv::Mat depthImage(HEIGHT, WIDTH, CV_8UC1);

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            int idx = i * WIDTH + j;
            depthImage.at<uint8_t>(i, j) = depth.d[idx];
        }
    }

    // 이미지 표시
    cv::imshow("Depth Image", depthImage);
    cv::waitKey(1); // 1ms 대기 (실시간 처리)
}
