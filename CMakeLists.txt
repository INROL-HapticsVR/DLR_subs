cmake_minimum_required(VERSION 3.10)
project(DLR_subs)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일과 헤더 파일 포함
set(SOURCE_FILES
    src/Test/Test1.cpp
    src/main.cpp
)

set(HEADER_FILES
    include/Test/Test1.hpp
)

# 실행 파일 생성
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# 헤더 파일 디렉토리 추가
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Mosquitto 라이브러리 설정
# 서브모듈 경로 지정
add_subdirectory(lib/mosquitto)

# Mosquitto 라이브러리 링크
target_link_libraries(${PROJECT_NAME} PRIVATE mosquitto)
