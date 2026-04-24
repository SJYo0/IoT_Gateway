#ifndef MCP3008_HPP
#define MCP3008_HPP

#include <string>
#include <cstdint>

class MCP3008 {
private:
    int spi_fd;
    std::string device_path;
    uint32_t speed;
    uint8_t mode;
    uint8_t bits_per_word;

public:
    // 라즈베리파이의 기본 SPI0 버스의 첫 번째 기기(CE0)를 기본값으로 사용
    MCP3008(const std::string& path = "/dev/spidev0.0", uint32_t speed_hz = 1000000);
    ~MCP3008();

    bool init();
    int readADC(int channel); // 0번 ~ 7번 채널 중 선택하여 읽기
};

#endif