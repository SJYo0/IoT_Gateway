#include "MqttManager.hpp"
#include "NetworkUtils.hpp"
#include <cstdlib> // system() 함수
#include <iostream>

// 생성자: 클라이언트 초기화
MqttManager::MqttManager(const std::string& address, const std::string& cid, const std::string& mac)
    : server_address(address), client_id(cid), mac_id(mac), client(address, cid) {
        client.set_callback(*this); // 본인이 이벤트 핸들러
    }

// 핸드쉐이크 로직 (디바이스 등록 요청 및 확인)
bool MqttManager::connect() {
    // LWT(유언장) 설정: 변수로 인한 다운시 OFFLINE 알림
    std::string willPayload = "{\"mac_address\": \"" + mac_id + "\", \"status\": \"OFFLINE\"}";
    mqtt::will_options willOpts("devices/status", willPayload, 1, false);

    mqtt::connect_options connOpts; // MQTT 연결 옵션
    connOpts.set_clean_session(true);
    connOpts.set_keep_alive_interval(20);
    connOpts.set_will(willOpts);

    try {
        std::cout << "[MQTT] MQTT 브로커 서버 연결 시도 중..." << std::endl;
        mqtt::token_ptr conntok = client.connect(connOpts);
        client.connect(connOpts)->wait();
        std::cout << "[MQTT] 연결 성공!" << std::endl;

        // (Subscribe)
        std::string my_response_topic = "provisioning/response/" + mac_id;
        client.subscribe(my_response_topic, 1)->wait();
        std::cout << "[MQTT] 'provisioning/response' 구독 완료. 백엔드 응답 대기 중..." << std::endl;

        // 기기 연결 승인 요청 Publish
        std::string my_ip = NetworkUtils::getIpAddress(NetworkUtils::getActiveInterfaceName());
        std::string reqPayload = "{\"mac_address\": \"" + mac_id + "\", \"ip_address\": \"" + my_ip + "\"}"; 
        publish("provisioning/request", reqPayload);
        std::cout << "[MQTT] 기기 승인 요청 발송 완료." << std::endl;
        
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT 오류] " << exc.what() << std::endl;
        return false;
    }
}

// 메시지 발행 함수
void MqttManager::publish(const std::string& topic, const std::string& payload) {
    try {
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
        pubmsg->set_qos(1); // 적어도 1번 전송보장
        client.publish(pubmsg)->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT 발행 오류] " << exc.what() << std::endl;
    }
}

// 정상 종료 처리
void MqttManager::disconnect() {
    try {
        std::cout << "\n[MQTT] 정상 종료: OFFLINE 메시지 전송 중..." << std::endl;
        std::string offlinePayload = "{\"mac_address\": \"" + mac_id + "\", \"status\": \"OFFLINE\"}";
        publish("devices/status", offlinePayload);

        // MediaMTX 서버 종료
        std::cout << "[MQTT] 카메라 스트리밍 서버(MediaMTX)를 종료합니다." << std::endl;
        system("sudo systemctl stop mediamtx");
        // system("sudo killall mediamtx");

        std::cout << "[MQTT] 브로커 서버 연결 해제 중..." << std::endl;
        client.disconnect()->wait();
        std::cout << "[MQTT] 게이트웨이 서버 종료." << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT] 오류발생: " << exc.what() << std::endl;
    }
}

// 콜백 함수들: 백엔드 메시지 수신
void MqttManager::message_arrived(mqtt::const_message_ptr msg) {
    std::string topic = msg->get_topic();
    std::string payload = msg->to_string();
    
    std::cout << "[메시지 수신] 토픽: " << topic << " | 내용: " << payload << std::endl;

    std::string my_response_topic = "provisioning/response/" + mac_id;
    // 본 기기에 온 응답 메시지인 경우에만 확인
    if (topic == my_response_topic) {
        // 연결 승인 시
        if (payload.find("APPROVED") != std::string::npos) {
            std::cout << "\n[MQTT] 기기 연결요청 승인 확인" << std::endl;
            is_approved = true; // 센서 스레드 동작 시작

            // MediaMTX 서버 가동
            std::cout << "[MQTT] MediaMTX 서버 구동" << std::endl;
            system("sudo systemctl start mediamtx"); 
            
            // system("/mediamtx/mediamtx &");
        }
        // 연결 거절 시
        else if (payload.find("REJECTED") != std::string::npos) {
            std::cout << "\n[MQTT] 기기 연결요청 거절 확인" << std::endl;
            std::cout << "\n[MQTT] 게이트웨이 서버를 종료 후, 다시 시도해주세요." << std::endl;
            std::cout << "\n(Ctrl+C를 눌러 종료)" << std::endl;
            is_approved = false;
        }
    }
}

// 브로커 서버 다운 시 알림
void MqttManager::connection_lost(const std::string& cause) {
    std::cout << "\n[MQTT] 연결이 끊어졌습니다: " << cause << std::endl;
}

void MqttManager::delivery_complete(mqtt::delivery_token_ptr token) {}