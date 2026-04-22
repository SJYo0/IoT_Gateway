cmake_minimum_required(VERSION 3.10)
project(IoT_Gateway)

# C++ 17 표준 사용 강제
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 헤더 파일
include_directories(include)

# src .cpp 파일을 소스로 지정
file(GLOB SOURCES "src/*.cpp")

# 실행 파일 생성
add_executable(gateway_app ${SOURCES})

# 라이브러리 링크
target_link_libraries(gateway_app
    paho-mqttpp3
    paho-mqtt3as
    pthread
)