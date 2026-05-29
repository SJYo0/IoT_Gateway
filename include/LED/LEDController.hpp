#ifndef LED_CONTROLLER_HPP
#define LED_CONTROLLER_HPP

#include <cstdint>
#include <gpiod.h> 
#include <thread> 
#include <atomic> 

// GPIO 핀
#define PIN_DATA  23 // DS
#define PIN_CLOCK 24 // SHCP
#define PIN_LATCH 25 // STCP
#define PIN_SPEAKER 18 // 스피커

enum LEDBit {
    NORTH_WINDOW = 0,
    SOUTH_WINDOW = 1,
    EAST_WINDOW = 2,
    WEST_WINDOW = 3,
    AIR_COND = 4,
    HEATING = 5,
    HUMIDIFIER = 6,
    DEHUMIDIFIER = 7,
    AIR_CLEANER = 8,
    SPRINKLER = 9,
    FIRE_ALARM = 10
};

class LEDController {
private:
    uint16_t now_state = 0; // LED 컨트롤 비트

    // 제어 포인터 객체
    struct gpiod_line_request *request_ptr = nullptr;

    std::thread alarm_thread;
    std::atomic<bool> is_alarm_active{false};

    void ledControl(uint16_t data);
    void runAlarmLoop(); // 스레드 함수
public:
    LEDController();
    ~LEDController();

    void updateLED(LEDBit bit, bool isON);

    void turnOffAll();
};

#endif