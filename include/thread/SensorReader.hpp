#ifndef SENSOR_READER_HPP
#define SENSOR_READER_HPP

#include "SensorDTO.hpp"
#include "ThreadSafeQueue.hpp"
#include "BME280.hpp"
#include "ENS160.hpp"
#include "MCP3008.hpp"
#include <thread>
#include <atomic>

class SensorReader {
private:
    BME280 bme;
    ENS160 ens;
    MCP3008 mcp;
    
    std::thread workerThread;
    std::atomic<bool> isRunning;
    ThreadSafeQueue<SensorDTO>& dataQueue;

    void readLoop(); // 스레드 동작 함수

public:
    SensorReader(ThreadSafeQueue<SensorDTO>& queue);
    ~SensorReader();

    bool start();
    void stop();
};

#endif