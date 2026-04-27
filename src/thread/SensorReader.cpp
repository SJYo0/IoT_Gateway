#include "SensorReader.hpp"
#include <iostream>
#include <chrono>

SensorReader::SensorReader(ThreadSafeQueue<SensorDTO>& queue) 
    : dataQueue(queue), isRunning(false) {}

SensorReader::~SensorReader() {
    stop();
}

bool SensorReader::start() {
    std::cout << "[SensorReader] 센서 초기화 시작" << std::endl;
    
    // 센서 초기화
    if (!bme.init() || !ens.init() || !mcp.init()) {
        std::cerr << "[SensorReader] 센서 초기화 실패" << std::endl;
        return false;
    }

    isRunning = true;
    // readLoop 함수 스레드 시작
    workerThread = std::thread(&SensorReader::readLoop, this);
    
    std::cout << "[SensorReader] 데이터 수집 스레드 가동" << std::endl;
    return true;
}

void SensorReader::stop() {
    if (isRunning) {
        isRunning = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
        std::cout << "[SensorReader] 데이터 수집 스레드 종료" << std::endl;
    }
}

void SensorReader::readLoop() {
    while (isRunning) {
        SensorDTO data;
        
        // BME280 센서값
        data.temperature = bme.readTemperature();
        data.humidity = bme.readHumidity();
        data.pressure = bme.readPressure();
        
        // 온습도 ENS160 전달
        ens.setEnvironmentData(data.temperature, data.humidity);

        // ENS160 센서값
        data.tvoc = ens.readTVOC();
        data.eco2 = ens.readECO2();
        
        // KY-026 with MCP3008 아날로그 값
        data.flameValue  = mcp.readADC(0); // 채널 0
        
        // 센서값 조회 시각
        auto now = std::chrono::system_clock::now();
        data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        dataQueue.push(data);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}