#include "MqttManager.hpp"
#include "NetworkUtils.hpp"
#include <cstdlib> // system() 함수
#include <iostream>

// 생성자: 클라이언트 초기화
MqttManager::MqttManager(const std::string& address, const std::string& cid, const std::string& mac)
    : server_address(address), client_id(cid), mac_id(mac), client(address, cid) {
        // 콜백 등록
        client.set_callback(*this);
    }

// 핸드쉐이크 (연결 및 프로비저닝)
bool MqttManager::connect() {
    // 유언장(LWT) 설정: 비정상 다운 시 브로커가 대신 뿌려줄 OFFLINE 메시지
    std::string willPayload = "{\"mac_address\": \"" + mac_id + "\", \"status\": \"OFFLINE\"}";
    mqtt::will_options willOpts("devices/status", willPayload, 1, false);

    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);
    connOpts.set_keep_alive_interval(20);
    connOpts.set_will(willOpts);

    try {
        std::cout << "[MQTT] 백엔드 브로커 연결 시도 중..." << std::endl;
        mqtt::token_ptr conntok = client.connect(connOpts);
        client.connect(connOpts)->wait();
        std::cout << "[MQTT] 연결 성공!" << std::endl;

        // (Subscribe)
        std::string my_response_topic = "provisioning/response/" + mac_id;
        client.subscribe(my_response_topic, 1)->wait();
        std::cout << "[MQTT] 'provisioning/response' 구독 완료. 백엔드 응답 대기 중..." << std::endl;

        // (Publish)
        std::string my_ip = NetworkUtils::getIpAddress(NetworkUtils::getActiveInterfaceName());
        std::string reqPayload = "{\"mac_address\": \"" + mac_id + "\", \"ip_address\": \"" + my_ip + "\"}"; 
        publish("provisioning/request", reqPayload);
        std::cout << "[MQTT] 승인 요청(Request) 발송 완료." << std::endl;
        
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT 오류] " << exc.what() << std::endl;
        return false;
    }
}

// 메시지 발행 (공통 함수)
void MqttManager::publish(const std::string& topic, const std::string& payload) {
    try {
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
        pubmsg->set_qos(1);
        client.publish(pubmsg)->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT 발행 오류] " << exc.what() << std::endl;
    }
}

// 정상 종료 시그널 처리
void MqttManager::disconnect() {
    try {
        std::cout << "\n[MQTT] 정상 종료: OFFLINE 메시지 전송 중..." << std::endl;
        std::string offlinePayload = "{\"mac_address\": \"" + mac_id + "\", \"status\": \"OFFLINE\"}";
        publish("devices/status", offlinePayload);

        // MediaMTX 서버 안전 종료!
        std::cout << "[시스템] 카메라 스트리밍 서버(MediaMTX)를 종료합니다." << std::endl;
        system("sudo systemctl stop mediamtx");
        // system("sudo killall mediamtx");

        std::cout << "[MQTT] 브로커와 안전하게 연결 해제 중..." << std::endl;
        client.disconnect()->wait();
        std::cout << "[MQTT] 게이트웨이 전원이 안전하게 차단되었습니다." << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT 종료 오류] " << exc.what() << std::endl;
    }
}

// 콜백 함수들: 백엔드 메시지 수신
void MqttManager::message_arrived(mqtt::const_message_ptr msg) {
    std::string topic = msg->get_topic();
    std::string payload = msg->to_string();
    
    std::cout << "[메시지 수신] 토픽: " << topic << " | 내용: " << payload << std::endl;

    std::string my_response_topic = "provisioning/response/" + mac_id;
    if (topic == my_response_topic) {
        // 이미 이 토픽으로 들어왔다는 것 자체가 나를 향한 메시지라는 뜻이므로,
        // 페이로드에서 굳이 mac_id를 또 찾을 필요 없이 APPROVED만 찾으면 됩니다!
        if (payload.find("APPROVED") != std::string::npos) {
            std::cout << "\n🎉 [프로비저닝 완료] 서버로부터 승인받았습니다!" << std::endl;
            is_approved = true;

            // MediaMTX 서버 가동
            std::cout << "[시스템] 카메라 스트리밍 서버(MediaMTX)를 시작합니다..." << std::endl;
            system("sudo systemctl start mediamtx"); 
            
            // system("/mediamtx/mediamtx &");
        }
        else if (payload.find("REJECTED") != std::string::npos) {
            std::cout << "\n[프로비저닝 실패] 서버로부터 거절당했습니다!" << std::endl;
            std::cout << "\n[System] 승인 거절 : 게이트웨이 서버를 종료 후, 다시 시도해주세요." << std::endl;
            is_approved = false;
        }
    }
}

void MqttManager::connection_lost(const std::string& cause) {
    std::cout << "\n[MQTT 경고] 연결이 끊어졌습니다: " << cause << std::endl;
}

void MqttManager::delivery_complete(mqtt::delivery_token_ptr token) {}