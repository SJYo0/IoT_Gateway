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
    // CE0 핀을 spi 스위치로 사용, 클럭 1MHz
    MCP3008(const std::string& path = "/dev/spidev0.0", uint32_t speed_hz = 1000000);
    ~MCP3008();

    bool init();
    int readADC(int channel); // 채널 선택 (0)
};

#endif