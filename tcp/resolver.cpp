#include "resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace net {

std::string address_resolution_error::describe() const {
    return gai_strerror(code);
};

std::expected<address_info, address_resolution_error> get_address_info(const std::string& address, uint16_t port, const addrinfo& hints) {
    addrinfo *res;
    if (auto status = getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &res); status != 0) {
        return std::unexpected{address_resolution_error{status}};
    }
    return address_info{res, freeaddrinfo};
}

std::expected<address_info, address_resolution_error> get_address_info(uint16_t port, const addrinfo& hints) {
    addrinfo *res;
    if (auto status = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &res); status != 0) {
        return std::unexpected{address_resolution_error{status}};
    }
    return address_info{res, freeaddrinfo};
}

}