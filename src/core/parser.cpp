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

Parser::Parser(int topic) : shm_fd(-1), shm_ptr(nullptr), shm_size(4 * WIDTH * HEIGHT * sizeof(uint8_t)) {
    topic_idx = std::to_string(topic);
    shm_name = "img_rendered_" + topic_idx;

    // Shared Memory 초기화
    shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("Failed to create shared memory.");
    }

    if (ftruncate(shm_fd, shm_size) == -1) {
        throw std::runtime_error("Failed to set shared memory size.");
    }

    shm_ptr = static_cast<uint8_t*>(mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (shm_ptr == MAP_FAILED) {
        throw std::runtime_error("Failed to map shared memory.");
    }
}

Parser::~Parser() {
    if (shm_ptr) {
        munmap(shm_ptr, shm_size);
        shm_ptr = nullptr;
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
    shm_unlink(shm_name.c_str());
}

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
        // std::cout << "[Debug] Parser time: " << topic_idx << ": " << duration.count() << " ms" << std::endl;
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
    if (!shm_ptr) {
        throw std::runtime_error("Shared memory pointer is null.");
    }

    std::memcpy(&time, &payload[0], sizeof(int64_t));

    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        shm_ptr[4 * i + 0] = payload[8 + 4 * i + 2]; // R
        shm_ptr[4 * i + 1] = payload[8 + 4 * i + 1]; // G
        shm_ptr[4 * i + 2] = payload[8 + 4 * i + 0]; // B
        shm_ptr[4 * i + 3] = payload[8 + 4 * i + 3]; // Depth
    }

    if (msync(shm_ptr, shm_size, MS_SYNC) == -1) {
        throw std::runtime_error("Failed to sync shared memory.");
    }
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


// #include "core/parser.hpp"

// #include <stdexcept>
// #include <cstring>
// #include <opencv2/opencv.hpp> // OpenCV의 모든 기본 모듈 포함
// #include <opencv2/highgui.hpp> // GUI 창을 위한 모듈
// #include <opencv2/imgproc.hpp> // 이미지 처리 모듈
// #include <sys/mman.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <sstream>
// #include <chrono>

// Parser::Parser(int topic) 
//     : shm_fd(-1), shm_ptr(nullptr), shm_size(4 * WIDTH * HEIGHT * sizeof(uint8_t)), activeBuffer(0) {
//     topic_idx = std::to_string(topic);
//     shm_name = "img_rendered_" + topic_idx;

//     // Shared Memory 초기화
//     shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
//     if (shm_fd == -1) {
//         throw std::runtime_error("Failed to create shared memory.");
//     }

//     if (ftruncate(shm_fd, shm_size * 2) == -1) { // 버퍼 2개를 위한 크기 설정
//         throw std::runtime_error("Failed to set shared memory size.");
//     }

//     shm_ptr = static_cast<uint8_t*>(mmap(0, shm_size * 2, PROT_WRITE, MAP_SHARED, shm_fd, 0));
//     if (shm_ptr == MAP_FAILED) {
//         throw std::runtime_error("Failed to map shared memory.");
//     }
// }

// Parser::~Parser() {
//     if (shm_ptr) {
//         munmap(shm_ptr, shm_size * 2);
//         shm_ptr = nullptr;
//     }
//     if (shm_fd != -1) {
//         close(shm_fd);
//         shm_fd = -1;
//     }
//     shm_unlink(shm_name.c_str());
// }

// void Parser::push(const std::string& payload) {
//     std::lock_guard<std::mutex> lock(dataMutex);
//     latestPayload = payload;              // 최신 데이터를 저장
//     dataCondition.notify_one();           // 소비 스레드 알림
// }

// std::string Parser::pop() {
//     std::unique_lock<std::mutex> lock(dataMutex);
//     dataCondition.wait(lock, [&]() { return !latestPayload.empty(); }); // 데이터가 없으면 대기
//     return latestPayload; // 가장 최근 데이터 반환
// }

// void Parser::consume() {
//     while (true) {
//         auto message = pop(); // 메시지 가져오기

//         auto tik = std::chrono::steady_clock::now();

//         parsing(message);

//         auto tak = std::chrono::steady_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(tak - tik);
//         std::cout << "[Debug] Parser time: " << topic_idx << ": " << duration.count() << " ms" << std::endl;
//     }
// }

// void Parser::parsing(std::string payload) {
//     #ifdef USE_RENDERING
//         //RGB 카메라
//         if (payload.size() == 8 + 3 * WIDTH * HEIGHT) {
//             throw std::runtime_error("RGB camera parsing not implemented.");
//         }

//         //RGB-D 카메라
//         else if (payload.size() == 8 + 4 * WIDTH * HEIGHT) {
//             // 데이터를 바이너리에서 구조체로 변환
//             std::memcpy(&time, &payload[0], sizeof(int64_t)); // 첫 8바이트를 int64_t로 변환

//             // 하림씨가 BGR로 준다도르
//             for (int i = 0; i < WIDTH * HEIGHT; i++) {
//                 rgb.r[i] = static_cast<uint8_t>(payload[8 + 4 * i + 2]);
//                 rgb.g[i] = static_cast<uint8_t>(payload[8 + 4 * i + 1]);
//                 rgb.b[i] = static_cast<uint8_t>(payload[8 + 4 * i + 0]);
//                 depth.d[i] = static_cast<uint8_t>(payload[8 + 4 * i + 3]);
//             }

//             // 이미지 표시
//             displayRGBImage();
//             displayDepthImage();
//         } else {
//             throw std::runtime_error("Invalid payload size.");
//         }

//     #else
//         if (!shm_ptr) {
//             throw std::runtime_error("Shared memory pointer is null.");
//         }

//         std::memcpy(&time, &payload[0], sizeof(int64_t));

//         // 현재 활성화된 버퍼로 데이터 쓰기
//         uint8_t* buffer = shm_ptr + (activeBuffer * shm_size);

//         for (int i = 0; i < WIDTH * HEIGHT; ++i) {
//             buffer[4 * i + 0] = payload[8 + 4 * i + 2]; // R
//             buffer[4 * i + 1] = payload[8 + 4 * i + 1]; // G
//             buffer[4 * i + 2] = payload[8 + 4 * i + 0]; // B
//             buffer[4 * i + 3] = payload[8 + 4 * i + 3]; // Depth
//         }

//         if (msync(buffer, shm_size, MS_SYNC) == -1) {
//             throw std::runtime_error("Failed to sync shared memory.");
//         }

//         // 활성 버퍼 전환
//         activeBuffer = (activeBuffer + 1) % 2;
//     #endif
// }

// void Parser::displayRGBImage() {
//     cv::Mat image(HEIGHT, WIDTH, CV_8UC3);

//     for (int i = 0; i < HEIGHT; ++i) {
//         for (int j = 0; j < WIDTH; ++j) {
//             int idx = i * WIDTH + j;
//             image.at<cv::Vec3b>(i, j) = cv::Vec3b(rgb.b[idx], rgb.g[idx], rgb.r[idx]);
//         }
//     }

//     cv::imshow("RGB Image " + topic_idx, image);
//     cv::waitKey(1); // 1ms 대기 (실시간 처리)
// }

// void Parser::displayDepthImage() {
//     cv::Mat depthImage(HEIGHT, WIDTH, CV_8UC1);

//     for (int i = 0; i < HEIGHT; ++i) {
//         for (int j = 0; j < WIDTH; ++j) {
//             int idx = i * WIDTH + j;
//             depthImage.at<uint8_t>(i, j) = depth.d[idx];
//         }
//     }

//     cv::imshow("Depth Image " + topic_idx, depthImage);
//     cv::waitKey(1); // 1ms 대기 (실시간 처리)
// }
