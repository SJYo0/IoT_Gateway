#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include "MqttManager.hpp"
#include "NetworkUtils.hpp"
#include "SensorReader.hpp"
#include "ThreadSafeQueue.hpp"
#include "SensorDTO.hpp"

// 시그널 핸들러에서 사용
MqttManager* global_mqtt_ptr = nullptr;
SensorReader* global_sensorReader_ptr = nullptr;

// OS가 종료 신호(Ctrl+C 등)를 보낼 때 가로채는 함수
void signalHandler(int signum) {
    std::cout << "\n[System] 강제 종료 시그널(" << signum << ") 감지" << std::endl;
    if (global_mqtt_ptr != nullptr) {
        global_mqtt_ptr->disconnect();
    }
    if (global_sensorReader_ptr != nullptr) {
        global_sensorReader_ptr->stop();
    }
    exit(signum);
}

std::string serializeToJson(const SensorDTO& data) {
    char buffer[256]; 
    
    snprintf(buffer, sizeof(buffer),
             "{\"temperature\":%.1f,"
             "\"humidity\":%.1f,"
             "\"pressure\":%.1f,"
             "\"tvoc\":%u,"
             "\"eco2\":%u,"
             "\"flameValue\":%d,"
             "\"timestamp\":%lld}",
             data.temperature, data.humidity, data.pressure, 
             data.tvoc, data.eco2, data.flameValue, data.timestamp);
             
    return std::string(buffer);
}

int main(int argc, char* argv[]) {
    
    if(argc != 3) {
        std::cout << "[System] 사용법 : " << argv[0] << " 백엔드IP 백엔드Port" << '\n';
        std::cout << "[System] 예시   : ./gateway_app 192.168.0.0 1883\n";
        return 1;
    }
    
    // 1. 종료 시그널 등록
    signal(SIGINT, signalHandler);  // Ctrl+C
    signal(SIGTERM, signalHandler); // 프로세스 kill

    // 기기 정보 동적 할당
    std::string active_iface = NetworkUtils::getActiveInterfaceName();
    std::string my_mac = NetworkUtils::getMacAddress(active_iface);
    std::string my_ip = NetworkUtils::getIpAddress(active_iface);

    std::cout << "[System] 활성 네트워크: " << active_iface << std::endl;
    std::cout << "[System] 내 MAC 주소: " << my_mac << std::endl;
    std::cout << "[System] 내 IP 주소: " << my_ip << std::endl;

    // 기기 Mac주소 전달
    std::string backend_ip = std::string("tcp://") + argv[1] + ":" + argv[2];
    MqttManager mqttManager(backend_ip, "gateway_" + my_mac, my_mac);
    global_mqtt_ptr = &mqttManager;

    // 핸드쉐이크
    if (!mqttManager.connect()) {
        std::cerr << "[System] 게이트웨이 백엔드 연결 실패. 프로그램을 종료합니다." << std::endl;
        return 1;
    }

    std::cout << "[System] 게이트웨이 디바이스 승인 대기중...." << std::endl;

    // 백엔드 승인 메시지 수신 대기
    while (!mqttManager.is_approved) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // 승인 완료 로직
    std::cout << "\n=============================================" << std::endl;
    std::cout << "[System] 센서 데이터 수집 스레드 가동" << std::endl;
    std::cout << "=============================================\n" << std::endl;
    
    // 4. 스레드 안전 큐 및 센서 스레드 가동 (TODO 영역 완성)
    ThreadSafeQueue<SensorDTO> dataQueue;
    SensorReader sensorManager(dataQueue);
    global_sensorReader_ptr = &sensorManager;

    if (!sensorManager.start()) {
        std::cerr << "[System] 하드웨어 초기화 실패. 프로그램을 종료합니다." << std::endl;
        mqttManager.disconnect();
        return 1;
    }

    // 5. 메인 루프 (소비자 역할: 큐에서 데이터 꺼내서 MQTT 전송)
    std::cout << "[System] 센서 데이터 수집 시작" << std::endl;
    while (1) {
        // 큐에서 수집된 센서 데이터 빼오기
        SensorDTO data = dataQueue.pop();

        std::cout << "[BME280] temp:" << data.temperature << "C, hum:" << data.humidity << "%, pres:" << data.pressure << "hPa" << std::endl;
        std::cout << "[ENS160] TVOC:" << data.tvoc << "ppb, ECO2:" << data.eco2 << "ppm" << std::endl;
        std::cout << "[KY-026] Flame:" << data.flameValue << std::endl << std::endl;

        // 센서 데이터 json 직렬화 및 발행
        std::string json = serializeToJson(data);
        mqttManager.publish("gateway/" + my_mac + "/telemetry", json);
    }

    return 0;
}