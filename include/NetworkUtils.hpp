#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

#include <string>

namespace NetworkUtils {
    // 💡 인터페이스 이름(예: wlan0, eth0)을 넣으면 해당 MAC 주소와 IP를 반환합니다.
    std::string getMacAddress(const std::string& interface_name);
    std::string getIpAddress(const std::string& interface_name);
    
    // 사용 가능한(연결된) 첫 번째 인터페이스(wlan0 또는 eth0)의 이름을 찾습니다.
    std::string getActiveInterfaceName();
}

#endif