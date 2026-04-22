#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include "MqttManager.hpp"
#include "NetworkUtils.hpp"

// 시그널 핸들러에서 사용
MqttManager* global_mqtt_ptr = nullptr;

// OS가 종료 신호(Ctrl+C 등)를 보낼 때 가로채는 함수
void signalHandler(int signum) {
    std::cout << "\n[System] 강제 종료 시그널(" << signum << ") 감지" << std::endl;
    if (global_mqtt_ptr != nullptr) {
        global_mqtt_ptr->disconnect();
    }
    exit(signum);
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
    
    // TODO: 센서 스레드 코드

    // 4. 메인 루프 (현재는 통신 연결만 유지하며 대기)
    std::cout << "[System] 게이트웨이 정상 가동 중... (Ctrl+C를 눌러 종료)" << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}