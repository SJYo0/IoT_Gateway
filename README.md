# 🌐 IoT Gateway (스마트 안전관리 AIoT 플랫폼)

본 프로젝트는 리눅스 환경에서 다양한 센서 데이터를 실시간으로 수집하고, MQTT 프로토콜을 통해 백엔드 서버와 통신하며, 원격으로 하드웨어를 제어하는 IoT 게이트웨이 애플리케이션입니다. 

## 주요 기능
* **센서 데이터 실시간 수집**: 멀티스레드 기반으로 온도, 습도, 기압, 공기질, 화재 감지 데이터를 1초 주기로 수집합니다.
* **MQTT 비동기 통신 및 자동 프로비저닝**: 게이트웨이 실행 시 기기의 MAC 및 IP 주소를 추출하여 백엔드에 자동 등록을 요청하고 데이터를 전송합니다.
* **하드웨어 원격 제어 및 알람**: 백엔드에서 수신된 JSON 제어 명령을 파싱하여 하드웨어를 제어하고, 화재 감지 시 즉각적인 경보 알람을 발생시킵니다.
* **실시간 영상 스트리밍**: 프로비저닝 완료 시 `rpicam-vid`와 `ffmpeg`를 활용하여 실시간 비디오 스트리밍을 송출합니다.
* **안정적인 시스템 종료(Graceful Shutdown)**: 비정상 종료 시에도 인프라 자원을 안전하게 반환합니다.

## 기술 스택

| 분류 | 기술 및 스택 | 설명 |
| :--- | :--- | :--- |
| **Language** | C++ (C++17) | 최신 C++ 표준을 활용한 시스템 프로그래밍 |
| **Build System** | CMake | 프로젝트 빌드 및 의존성 관리 |
| **Network / Protocol** | MQTT, RTSP | Paho MQTT C++ 라이브러리를 활용한 비동기 메시징 및 영상 스트리밍 |
| **Hardware Bus** | I2C, SPI, GPIO | 센서 및 액추에이터 통신 인터페이스 |
| **Sensors** | BME280, ENS160, MCP3008, KY-026 | 온습도/기압, 대기질, 아날로그-디지털 변환, 화재감지 |

## 디렉토리 구조

```text
IoT_Gateway/
├── CMakeLists.txt               # CMake 빌드 구성 파일
├── README.md                    # 프로젝트 가이드
├── .gitignore                   # Git 추적 제외 파일 목록
├── include/                     # 헤더 파일 (.hpp)
│   ├── LED/                     
│   │   └── LEDController.hpp   
│   ├── network/                 
│   │   ├── MqttManager.hpp     
│   │   └── NetworkUtils.hpp    
│   ├── sensor/                  
│   │   ├── BME280.hpp          
│   │   ├── ENS160.hpp          
│   │   └── MCP3008.hpp         
│   └── thread/                  
│       ├── SensorDTO.hpp       
│       ├── SensorReader.hpp    
│       └── ThreadSafeQueue.hpp 
└── src/                         # 소스 파일 (.cpp)
    ├── main.cpp                
    ├── LED/                     
    │   └── LEDController.cpp   
    ├── network/
    │   ├── MqttManager.cpp     
    │   └── NetworkUtils.cpp    
    ├── sensor/                  
    │   ├── BME280.cpp          
    │   ├── ENS160.cpp          
    │   └── MCP3008.cpp         
    └── thread/
        └── SensorReader.cpp    
```

## 빌드 및 실행 방법

### 1. 사전 요구 사항 (Dependencies)
* CMake (3.10 이상)
* GCC / G++ (C++17 지원 컴파일러)
* Eclipse Paho MQTT C++ Library (`libpaho-mqtt-cpp`)
* libgpiod (`libgpiod-dev`)
* nlohmann-json (`nlohmann-json3-dev`)

### 2. 빌드 (Build)
```bash
mkdir build
cd build
cmake ..
make -j4
```

### 3. 실행 (Usage)
```bash
# ./gateway_app [브로커IP] [브로커Port]
./gateway_app 192.168.0.0 1883
```