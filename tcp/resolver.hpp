#pragma once

#include <netdb.h>

#include <string>
#include <memory>
#include <expected>

namespace net {

using address_info = std::unique_ptr<addrinfo, void(*)(addrinfo*)>;

struct address_resolution_error {
    int code;

    std::string describe() const;
};

std::expected<address_info, address_resolution_error> get_address_info(const std::string& address, uint16_t port, const addrinfo& hints);
std::expected<address_info, address_resolution_error> get_address_info(uint16_t port, const addrinfo& hints);

}