#include "tcp.hpp"

#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>

#include <cstring>
#include <utility>
#include <string_view>

namespace net {


tcp_sock::tcp_sock(tcp_sock&& other) noexcept : fd{std::exchange(other.fd, -1)} {}
tcp_sock& tcp_sock::operator=(tcp_sock other) {
    swap(other);
    return *this;
}

void tcp_sock::swap(tcp_sock& other) {
    std::swap(fd, other.fd);
}

tcp_sock::~tcp_sock() {
    if (fd != -1)
        ::close(fd);
}

std::expected<size_t, recv_error> tcp_sock::recv(std::span<char> view) {
    auto result = ::recv(fd, view.data(), view.size(), 0);
    if (result == -1) {
        return std::unexpected{recv_error{errno}};
    }
    return static_cast<size_t>(result);
}

std::expected<size_t, recv_error> tcp_sock::send(std::span<char> view) {
    auto result = ::send(fd, view.data(), view.size(), 0);
    if (result == -1) {
        return std::unexpected{recv_error{errno}};
    }
    return static_cast<size_t>(result);
}

tcp_sock::tcp_sock(int fd_): fd{fd_} {}

std::expected<tcp_sock, std::variant<socket_error, connect_error>>
tcp_sock::make_sock(address_info info) {
    using err_type = std::variant<socket_error, connect_error>;
    auto fd = socket(info -> ai_family, info -> ai_socktype, info -> ai_protocol);
    if (fd == -1) {
        return std::unexpected{err_type{socket_error{errno}}};
    }
    if (auto status = ::connect(fd, info -> ai_addr, info -> ai_addrlen); status == -1) {
        ::close(fd);
        return std::unexpected{err_type{connect_error{errno}}};
    }
    ::fcntl(fd, F_SETFL, O_NONBLOCK);
    return tcp_sock{fd};
}

auto get_sock_description (const sockaddr& addr) -> tcp_server_nb::sock_description {
    char address_buffer[INET6_ADDRSTRLEN];
    switch (addr.sa_family) {
        case AF_INET: {
            auto* addr_cast = reinterpret_cast<sockaddr_in*>(address_buffer);
            ::inet_ntop(AF_INET, &(addr_cast -> sin_addr), address_buffer, sizeof(address_buffer));
            uint16_t port = htons(addr_cast -> sin_port);
            return {{std::string(address_buffer), port}};
        }
        case AF_INET6: {
            auto* addr_cast = reinterpret_cast<sockaddr_in6*>(address_buffer);
            ::inet_ntop(AF_INET6, &(addr_cast -> sin6_addr), address_buffer, sizeof(address_buffer));
            uint16_t port = htons(addr_cast -> sin6_port);
            return {{std::string(address_buffer), port}};
        }
        default:
            return {{"", 0}};
    }
}


std::expected<std::pair<tcp_sock, tcp_server_nb::sock_description>, accept_error> tcp_server_nb::accept() {
    sockaddr addr;
    socklen_t len = sizeof(addr);
    // todo: add addr and len to tcp_sock, so we can query this data we get here
    int new_fd = ::accept(fd, &addr, &len);
    if (new_fd == -1)
        return std::unexpected{accept_error{errno}};
    ::fcntl(new_fd, F_SETFL, O_NONBLOCK);
    return {{tcp_sock{new_fd}, get_sock_description(addr)}};
}

tcp_server_nb::tcp_server_nb(tcp_server_nb&& other): fd{std::exchange(other.fd, -1)} {}

tcp_server_nb& tcp_server_nb::operator=(tcp_server_nb other) {
    swap(other);
    return *this;
}

void tcp_server_nb::swap(tcp_server_nb& other) {
    std::swap(fd, other.fd);
}

tcp_server_nb::~tcp_server_nb() {
    if (fd != -1) {
        ::close(fd);
    }
}

tcp_server_nb::tcp_server_nb(int fd_): fd{fd_} {}

std::expected<tcp_server_nb, std::variant<socket_error, bind_error, listen_error>>
tcp_server_nb::make_server(address_info info) {
    using err_type = std::variant<socket_error, bind_error, listen_error>;
    auto fd = socket(info -> ai_family, info -> ai_socktype, info -> ai_protocol);

    if (fd == -1) {
        return std::unexpected{err_type{socket_error{errno}}};
    }

    if (::bind(fd, info -> ai_addr, info -> ai_addrlen) == -1) {
        ::close(fd);
        return std::unexpected{err_type{bind_error{errno}}};
    }

    if (::listen(fd, listen_backlog) == -1) {
        ::close(fd);
        return std::unexpected{err_type{listen_error{errno}}};
    }

    ::fcntl(fd, F_SETFL, O_NONBLOCK);
    return tcp_server_nb{fd};
}

}
