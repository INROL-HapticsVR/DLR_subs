#include "core/parser.hpp"

#include <stdexcept>
#include <cstring>
#include <opencv2/opencv.hpp> // OpenCV의 모든 기본 모듈 포함
#include <opencv2/highgui.hpp> // GUI 창을 위한 모듈
#include <opencv2/imgproc.hpp> // 이미지 처리 모듈
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

#include <chrono>

Parser::Parser(int topic) {
    topic_idx = std::to_string(topic);
}

Parser::~Parser() {}

void Parser::push(const std::string& payload) {
    std::lock_guard<std::mutex> lock(dataMutex);
    // 가장 최근 데이터 저장
    latestPayload = payload;
    dataCondition.notify_one(); // 소비 스레드에 알림
}

std::string Parser::pop() {
    std::unique_lock<std::mutex> lock(dataMutex);
    dataCondition.wait(lock, [&]() { return !latestPayload.empty(); }); // 데이터가 비어있으면 대기
    return latestPayload; // 가장 최근 데이터 반환
}

void Parser::consume() {
    while (true) {
        auto message = pop(); // 가장 최근 메시지 가져오기

        auto tik = std::chrono::steady_clock::now();

        parsing(message);

        auto tak = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(tak - tik);
        std::cout << "[Debug] Parser time: " << topic_idx << ": " << duration.count() << " ms" << std::endl;
    }
}

void Parser::parsing(std::string payload) {
#ifdef USE_RENDERING
    //RGB 카메라
    if (payload.size() == 8 + 3 * WIDTH * HEIGHT) {
        throw std::runtime_error("RGB camera parsing not implemented.");
    }

    //RGB-D 카메라
    else if (payload.size() == 8 + 4 * WIDTH * HEIGHT) {
        // 데이터를 바이너리에서 구조체로 변환
        std::memcpy(&time, &payload[0], sizeof(int64_t)); // 첫 8바이트를 int64_t로 변환

        // 하림씨가 BGR로 준다도르
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            rgb.r[i] = static_cast<uint8_t>(payload[8 + 4 * i + 2]);
            rgb.g[i] = static_cast<uint8_t>(payload[8 + 4 * i + 1]);
            rgb.b[i] = static_cast<uint8_t>(payload[8 + 4 * i + 0]);
            depth.d[i] = static_cast<uint8_t>(payload[8 + 4 * i + 3]);
        }

        // 이미지 표시
        displayRGBImage();
        displayDepthImage();
    } else {
        throw std::runtime_error("Invalid payload size.");
    }
#else
    // 데이터를 바이너리에서 구조체로 변환
    std::memcpy(&time, &payload[0], sizeof(int64_t)); // 첫 8바이트를 int64_t로 변환

    // Shared memory 이름
    std::string shm_name = "img_rendered_" + topic_idx;

    // Shared memory 생성 및 설정
    int shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("Failed to create shared memory.");
    }

    size_t shm_size = sizeof(image_shared);
    if (ftruncate(shm_fd, shm_size) == -1) {
        throw std::runtime_error("Failed to set shared memory size.");
    }

    // Shared memory 매핑
    void* shm_ptr = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        throw std::runtime_error("Failed to map shared memory.");
    }

    // Shared memory 데이터 저장
    auto* shared_img = static_cast<image_shared*>(shm_ptr);

    // 하림씨가 BGR로 준다도르
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        shared_img->r[i] = static_cast<uint8_t>(payload[8 + 4 * i + 2]);  // R
        shared_img->g[i] = static_cast<uint8_t>(payload[8 + 4 * i + 1]);  // G
        shared_img->b[i] = static_cast<uint8_t>(payload[8 + 4 * i + 0]);  // B
        shared_img->depth[i] = static_cast<uint8_t>(payload[8 + 4 * i + 3]);  // Depth
    }

    // Shared memory 동기화 및 정리
    if (msync(shm_ptr, shm_size, MS_SYNC) == -1) {
        throw std::runtime_error("Failed to sync shared memory.");
    }

    if (munmap(shm_ptr, shm_size) == -1) {
        throw std::runtime_error("Failed to unmap shared memory.");
    }

    close(shm_fd);
#endif
}

void Parser::displayRGBImage() {
    cv::Mat image(HEIGHT, WIDTH, CV_8UC3);

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            int idx = i * WIDTH + j;
            image.at<cv::Vec3b>(i, j) = cv::Vec3b(rgb.b[idx], rgb.g[idx], rgb.r[idx]);
        }
    }

    cv::imshow("RGB Image " + topic_idx, image);
    cv::waitKey(1); // 1ms 대기 (실시간 처리)
}

void Parser::displayDepthImage() {
    cv::Mat depthImage(HEIGHT, WIDTH, CV_8UC1);

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            int idx = i * WIDTH + j;
            depthImage.at<uint8_t>(i, j) = depth.d[idx];
        }
    }

    cv::imshow("Depth Image " + topic_idx, depthImage);
    cv::waitKey(1); // 1ms 대기 (실시간 처리)
}

