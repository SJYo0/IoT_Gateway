#include "MqttManager.hpp"
#include "NetworkUtils.hpp"
#include <cstdlib> // system() 함수
#include <iostream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
    connOpts.set_automatic_reconnect(1, 10);

    try {
        std::cout << "[MQTT] MQTT 브로커 서버 연결 시도 중..." << std::endl;
        mqtt::token_ptr conntok = client.connect(connOpts);
        conntok->wait();
        std::cout << "[MQTT] 연결 성공!" << std::endl;

        // (Subscribe)
        std::string my_response_topic = "provisioning/response/" + mac_id;
        client.subscribe(my_response_topic, 1)->wait();

        std::string my_control_topic = "webbackend/control/" + mac_id;
        client.subscribe(my_control_topic, 1)->wait();
        
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

        std::cout << "[MQTT] 모든 인프라를 종료합니다." << std::endl;
        ledController.turnOffAll();

        // MediaMTX 서버 종료
        std::cout << "[MQTT] 카메라 송출을 종료합니다." << std::endl;
        system("pkill -15 -f 'rpicam-vid'; pkill -15 -f 'ffmpeg' 2>/dev/null");
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
    std::string my_control_topic = "webbackend/control/" + mac_id;
    // 본 기기에 온 응답 메시지인 경우에만 확인
    if (topic == my_response_topic) {
        // 연결 승인 시
        if (payload.find("APPROVED") != std::string::npos) {
            std::cout << "\n[MQTT] 기기 연결요청 승인 확인" << std::endl;
            is_approved = true; // 센서 스레드 동작 시작

            std::cout << "[MQTT] 모든 인프라를 초기 상태로 초기화합니다." << std::endl;
            ledController.turnOffAll();

            // MediaMTX 서버에 영상 송출
            std::string streamPath = mac_id;
            streamPath.erase(
                std::remove(streamPath.begin(), streamPath.end(), ':'),
                streamPath.end()
            );

            std::cout << "[MQTT] MediaMTX 서버 영상 송출 시작 : " << streamPath <<std::endl;

            std::string cmd =
                "bash -c 'while true; do "
                "rpicam-vid -t 0 --inline "
                "--width 640 --height 480 "
                "--framerate 15 --intra 15 "
                "--codec h264 --profile baseline "
                "--libav-format h264 -o - | "
                "ffmpeg -f h264 -i - "
                "-c:v copy "
                "-rtsp_transport tcp "
                "-f rtsp "
                "rtsp://210.104.76.141:8554/" + streamPath +
                "; echo \"[System] 송출 재시작...\"; "
                "sleep 2; "
                "done' &";

            system(cmd.c_str());
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
    else if (topic == my_control_topic) {
        try {
            json j = json::parse(payload);
            std::cout << "\n[MQTT] 하드웨어 제어 명령 파싱 및 실행..." << std::endl;

            // 키 값이 존재하고, boolean 타입일 때만 상태를 업데이트
            if (j.contains("north_window") && j["north_window"].is_boolean())
                ledController.updateLED(NORTH_WINDOW, j["north_window"].get<bool>());
                
            if (j.contains("south_window") && j["south_window"].is_boolean())
                ledController.updateLED(SOUTH_WINDOW, j["south_window"].get<bool>());
                
            if (j.contains("east_window") && j["east_window"].is_boolean())
                ledController.updateLED(EAST_WINDOW, j["east_window"].get<bool>());
                
            if (j.contains("west_window") && j["west_window"].is_boolean())
                ledController.updateLED(WEST_WINDOW, j["west_window"].get<bool>());
                
            if (j.contains("air_conditioner") && j["air_conditioner"].is_boolean())
                ledController.updateLED(AIR_COND, j["air_conditioner"].get<bool>());
                
            if (j.contains("heating") && j["heating"].is_boolean())
                ledController.updateLED(HEATING, j["heating"].get<bool>());
                
            if (j.contains("humidifier") && j["humidifier"].is_boolean())
                ledController.updateLED(HUMIDIFIER, j["humidifier"].get<bool>());
                
            if (j.contains("dehumidifier") && j["dehumidifier"].is_boolean())
                ledController.updateLED(DEHUMIDIFIER, j["dehumidifier"].get<bool>());
                
            if (j.contains("air_cleaner") && j["air_cleaner"].is_boolean())
                ledController.updateLED(AIR_CLEANER, j["air_cleaner"].get<bool>());
                
            if (j.contains("sprinkler") && j["sprinkler"].is_boolean())
                ledController.updateLED(SPRINKLER, j["sprinkler"].get<bool>());
                
            if (j.contains("fire_alarm") && j["fire_alarm"].is_boolean())
                ledController.updateLED(FIRE_ALARM, j["fire_alarm"].get<bool>());

        } catch (const json::parse_error& e) {
            std::cerr << "[MQTT 오류] JSON 파싱 실패 (제어 명령): " << e.what() << std::endl;
        }
    }
}

// 브로커 서버 다운 시 알림
void MqttManager::connection_lost(const std::string& cause) {
    std::cout << "\n[MQTT] 연결이 끊어졌습니다: " << cause << std::endl;
}

void MqttManager::delivery_complete(mqtt::delivery_token_ptr token) {}