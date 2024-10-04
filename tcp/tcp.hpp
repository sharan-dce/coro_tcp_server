#pragma once

#include "resolver.hpp"

#include <sys/fcntl.h>
#include <sys/errno.h>
#include <unistd.h>

#include <utility>
#include <string>
#include <span>
#include <cstring>
#include <cstdint>
#include <variant>

namespace net {

template <typename T>
struct retryable_error_t {
    int code;

    bool retryable() const {
        return (code == EAGAIN) || (code == EWOULDBLOCK);
    }
};

struct socket_error { int code; };
struct connect_error { int code; };
struct bind_error { int code; };
struct listen_error { int code; };

struct accept_error_t {};
using accept_error = retryable_error_t<accept_error_t>;

struct recv_error_t {};
using recv_error = retryable_error_t<recv_error_t>;

using tcp_sock_error = std::variant<address_resolution_error, socket_error, connect_error>;
using tcp_server_error = std::variant<address_resolution_error, socket_error, bind_error, listen_error>;

struct tcp_ipv4 {
    constexpr static int family = AF_INET;
    constexpr static int protocol = PF_INET;
    constexpr static int sock_type = SOCK_STREAM;
};

struct tcp_ipv6 {
    constexpr static int family = AF_INET6;
    constexpr static int protocol = PF_INET6;
    constexpr static int sock_type = SOCK_STREAM;
};

struct tcp_unspec {
    constexpr static int family = AF_UNSPEC;
    constexpr static int protocol = PF_UNSPEC;
    constexpr static int sock_type = SOCK_STREAM;
};

template <typename T>
concept connection_spec = std::is_same_v<std::decay_t<decltype(T::family)>, int> &&
    std::is_same_v<std::decay_t<decltype(T::protocol)>, int> &&
    std::is_same_v<std::decay_t<decltype(T::sock_type)>, int>;

// non blocking tcp socket
class tcp_sock {
public:
    template <connection_spec spec>
    static std::expected<tcp_sock, tcp_sock_error> make_sock(const std::string destination_address, uint16_t port) {
        addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = spec::family;
        hints.ai_protocol = spec::protocol;
        hints.ai_socktype = spec::sock_type;

        auto look_up = get_address_info(destination_address, port, hints);

        return look_up ?
                make_sock<spec>(std::move(look_up.value())).transform_error([](auto verror) {
                    return std::visit([](auto error) { return tcp_sock_error{error}; }, verror); }) :
                std::unexpected{look_up.error()};
    }

    tcp_sock(tcp_sock&& other) noexcept;
    tcp_sock(const tcp_sock&) = delete;

    tcp_sock& operator=(tcp_sock other);

    void swap(tcp_sock& other);

    ~tcp_sock();

    std::expected<size_t, recv_error> recv(std::span<char> view);

    std::expected<size_t, recv_error> send(std::span<char> view);

private:
    friend class tcp_server_nb;

    tcp_sock(int fd_);

    static std::expected<tcp_sock, std::variant<socket_error, connect_error>>
    make_sock(address_info info);

    int fd = -1;
};

class tcp_server_nb {
public:
    template <connection_spec spec>
    static std::expected<tcp_server_nb, tcp_server_error> make_server(uint16_t port) {
        addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = spec::family;
        hints.ai_protocol = spec::protocol;
        hints.ai_socktype = spec::sock_type;
        hints.ai_flags = AI_PASSIVE;

        auto look_up = get_address_info(port, hints);
        if (!look_up) {
            return std::unexpected{look_up.error()};
        }

        auto server = make_server(std::move(look_up.value()));

        using r_type = std::expected<tcp_server_nb, tcp_server_error>;
        return server ?
            r_type(std::move(server.value())) :
            r_type(std::unexpect_t{},
                   std::visit([](auto error) -> tcp_server_error { return error; }, server.error()));
    }

    struct sock_description: std::pair<std::string, uint16_t> {};
    std::expected<std::pair<tcp_sock, sock_description>, accept_error> accept();

    tcp_server_nb(tcp_server_nb&& other);
    tcp_server_nb(const tcp_server_nb& other) = delete;

    tcp_server_nb& operator=(tcp_server_nb other);

    void swap(tcp_server_nb& other);

    ~tcp_server_nb();

private:
    constexpr static size_t listen_backlog = 128;

    tcp_server_nb(int fd_);

    static std::expected<tcp_server_nb, std::variant<socket_error, bind_error, listen_error>> make_server(address_info info);

    int fd = -1;
};

}
