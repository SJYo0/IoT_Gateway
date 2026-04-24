#include "SensorReader.hpp"
#include <iostream>
#include <chrono>

SensorReader::SensorReader(ThreadSafeQueue<SensorDTO>& queue) 
    : dataQueue(queue), isRunning(false) {}

SensorReader::~SensorReader() {
    stop();
}

bool SensorReader::start() {
    std::cout << "[SensorReader] 하드웨어 초기화 시작..." << std::endl;
    
    // 객체지향의 장점: 각 클래스의 init()만 호출하면 내부 복잡한 I2C/SPI 로직이 알아서 처리됨
    if (!bme.init() || !ens.init() || !mcp.init()) {
        std::cerr << "❌ 센서 초기화 실패! 스레드를 시작할 수 없습니다." << std::endl;
        return false;
    }

    isRunning = true;
    // readLoop 함수를 백그라운드 스레드로 분리하여 실행
    workerThread = std::thread(&SensorReader::readLoop, this);
    
    std::cout << "✅ [SensorReader] 데이터 수집 스레드 가동 완료." << std::endl;
    return true;
}

void SensorReader::stop() {
    if (isRunning) {
        isRunning = false;
        if (workerThread.joinable()) {
            workerThread.join(); // 스레드가 안전하게 끝날 때까지 대기
        }
        std::cout << "🛑 [SensorReader] 데이터 수집 스레드 종료." << std::endl;
    }
}

void SensorReader::readLoop() {
    while (isRunning) {
        SensorDTO data;
        
        // 1. 데이터 수집 (BME280은 온도를 먼저 읽어야 함)
        data.temperature = bme.readTemperature();
        data.humidity    = bme.readHumidity();
        data.pressure    = bme.readPressure();
        
        // 💡 2. 센서 퓨전 (Sensor Fusion) 
        // 방금 읽어온 정확한 온습도를 ENS160에 주입하여 내부 보정 알고리즘 갱신
        ens.setEnvironmentData(data.temperature, data.humidity);

        data.tvoc        = ens.readTVOC();
        data.eco2        = ens.readECO2();
        
        data.flameValue  = mcp.readADC(0); // CH0 (KY-026)
        
        // 현재 시간을 밀리초 단위로 기록
        auto now = std::chrono::system_clock::now();
        data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        // 2. 수집된 데이터를 스레드 안전 큐에 삽입 (Main 스레드로 전달)
        dataQueue.push(data);

        // 3. 1초 대기 (OS 스케줄러에게 CPU 양보)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}