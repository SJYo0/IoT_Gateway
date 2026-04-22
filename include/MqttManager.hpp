#ifndef MQTT_MANAGER_HPP
#define MQTT_MANAGER_HPP

#include <string>
#include <atomic> // 스레드간 원자성 보장
#include <mqtt/async_client.h>

// subscribe 에 비동기적 대응하기 위한 mqtt::callback 클래스
class MqttManager : public virtual mqtt::callback {
private:
    std::string server_address;
    std::string client_id;
    std::string mac_id;
    mqtt::async_client client;

    // mqtt::callback 필수 오버라이드
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr token) override;
    
public:

    // 메인의 while문이 사용할 플래그 (승인 or 미승인)
    std::atomic<bool> is_approved{false};
    
    // 생성자
    MqttManager(const std::string& address, const std::string& cid, const std::string& mac);
    
    // 핸드쉐이크 및 유언장 설정
    bool connect();
    
    // 정상 종료 처리
    void disconnect();
    
    // 메시지 발행 (나중에 센서 데이터를 보낼 때 사용)
    void publish(const std::string& topic, const std::string& payload);
};

#endif