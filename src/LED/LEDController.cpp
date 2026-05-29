#include "LEDController.hpp"
#include <iostream>
#include <stdexcept>
#include <unistd.h> // usleep 사용

LEDController::LEDController() {
    // GPIO 컨트롤러 open
    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip4");
    if (!chip) {
        throw std::runtime_error("[LED] gpiochip4를 열 수 없습니다.");
    }

    // 핀 설정 객체 생성
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    // 제어할 핀 번호 묶음 및 설정 관리 생성
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offsets[] = {PIN_DATA, PIN_CLOCK, PIN_LATCH, PIN_SPEAKER};
    gpiod_line_config_add_line_settings(line_cfg, offsets, 4, settings);

    // 요청 소비자 이름 등록
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "LED_Controller");

    // 제어 핸들
    request_ptr = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request_ptr) {
        throw std::runtime_error("[LED] 핀 제어 권한을 획득하지 못했습니다.");
    }

    // 설정 후 설정용 임시 객체들 삭제
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
    gpiod_chip_close(chip); // 칩도 반납

    // 초기 상태 쏘기
    ledControl(now_state);
}

LEDController::~LEDController() {
    is_alarm_active = false;
    if (alarm_thread.joinable()) {
        alarm_thread.join(); // 종료 시 스레드 정리
    }
    // 프로그램 종료 시 핀 점유 해제
    if (request_ptr) {
        gpiod_line_request_release(request_ptr);
    }
}

void LEDController::runAlarmLoop() {
    int delay_us = 625; // 울리는 주파수

    while (is_alarm_active) {
        // 1초 울림
        int beep_duration_us = 1000000; 
        
        // 울리는 반복문
        for (int i = 0; i < beep_duration_us; i += (delay_us * 2)) {
            if (!is_alarm_active) break;
            
            gpiod_line_request_set_value(request_ptr, PIN_SPEAKER, GPIOD_LINE_VALUE_ACTIVE);
            usleep(delay_us);
            
            gpiod_line_request_set_value(request_ptr, PIN_SPEAKER, GPIOD_LINE_VALUE_INACTIVE);
            usleep(delay_us);
        }

        // 0.5초 안울림
        if (is_alarm_active) {
            gpiod_line_request_set_value(request_ptr, PIN_SPEAKER, GPIOD_LINE_VALUE_INACTIVE);
            usleep(500000); 
        }
    }
}

void LEDController::ledControl(uint16_t data) {
    if (!request_ptr) return;

    // Latch LOW
    gpiod_line_request_set_value(request_ptr, PIN_LATCH, GPIOD_LINE_VALUE_INACTIVE);

    // 16비트 LED 제어 데이터 전송
    for (int i = 15; i >= 0; --i) {
        int bit = (data >> i) & 1;
        
        gpiod_line_request_set_value(request_ptr, PIN_DATA, bit ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
        
        // Clock Pulse
        gpiod_line_request_set_value(request_ptr, PIN_CLOCK, GPIOD_LINE_VALUE_ACTIVE);
        gpiod_line_request_set_value(request_ptr, PIN_CLOCK, GPIOD_LINE_VALUE_INACTIVE);
    }

    // Latch HIGH
    gpiod_line_request_set_value(request_ptr, PIN_LATCH, GPIOD_LINE_VALUE_ACTIVE);
}

void LEDController::updateLED(LEDBit bit, bool isON) {
    if (isON) {
        now_state |= (1 << bit);
    } else {
        now_state &= ~(1 << bit);
    }

    ledControl(now_state);

    if (bit == FIRE_ALARM) {
        if (isON) {
            if (!is_alarm_active) { // 이미 울리고 있을때 스레드 중복 생성 방지
                std::cout << "\n[LED] 화재 감지. 경보 시작" << std::endl;
                is_alarm_active = true;
                alarm_thread = std::thread(&LEDController::runAlarmLoop, this); // 스피커 스레드 동작
            }
        } else {
            if (is_alarm_active) {
                std::cout << "\n[LED] 화재 종료. 경보 중지" << std::endl;
                is_alarm_active = false;
                if (alarm_thread.joinable()) {
                    alarm_thread.join(); // 스피커 스레드 종료까지 대기
                }
                // 마지막에 스피커 확실히 끄기
                gpiod_line_request_set_value(request_ptr, PIN_SPEAKER, GPIOD_LINE_VALUE_INACTIVE);
            }
        }
    }
}

void LEDController::turnOffAll() {
    updateLED(NORTH_WINDOW, false);
    updateLED(SOUTH_WINDOW, false);
    updateLED(EAST_WINDOW, false);
    updateLED(WEST_WINDOW, false);
    updateLED(AIR_COND, false);
    updateLED(HEATING, false);
    updateLED(HUMIDIFIER, false);
    updateLED(DEHUMIDIFIER, false);
    updateLED(AIR_CLEANER, false);
    updateLED(SPRINKLER, false);
    updateLED(FIRE_ALARM, false);
}