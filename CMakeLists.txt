cmake_minimum_required(VERSION 3.10)
# 프로젝트 이름
project(IoT_Gateway)

# C++ 17 표준 사용 (최신 문법 활용)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 헤더 파일들이 모여있는 폴더 경로 지정
include_directories(include)

# src 폴더 안의 모든 .cpp 파일을 소스로 지정
file(GLOB SOURCES "src/*.cpp")

# 'gateway_app' 이라는 이름의 실행 파일 생성
add_executable(gateway_app ${SOURCES})

# 방금 설치한 라이브러리들(MQTT, 멀티스레드)을 실행 파일에 링크
target_link_libraries(gateway_app
    paho-mqttpp3
    paho-mqtt3as
    pthread
)