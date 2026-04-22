#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

#include <string>

namespace NetworkUtils {
    // wlan0 or eth0 삽입 / 와이파이 or 이더넷
    std::string getMacAddress(const std::string& interface_name);
    std::string getIpAddress(const std::string& interface_name);
    
    // 연결된 인터페이스 검색
    std::string getActiveInterfaceName();
}

#endif