#include "tcp/tcp.hpp"
#include "awaitables.hpp"
#include "server.hpp"
#include "coro/task.hpp"

#include <iostream>
#include <string_view>
#include <vector>
#include <span>

auto server_exception_callback = [](net::accept_error accept_error) -> void {
    std::cerr << "Non retryable server error code: " << accept_error.code << std::endl;
};

auto connection_exception_callback = [](auto ep) -> void {
    std::cerr << "Connection handling crashed!" << std::endl;
};

constexpr inline size_t TCP_BUFFER_SIZE = 4096;

coro::task<void> connection_handler(net::tcp_sock sock) {
    char buffer[TCP_BUFFER_SIZE];
    while (true) {
        if (auto recv = co_await net::make_socket_recv_awaitable(sock, {buffer, sizeof(buffer)}); recv && *recv) {
            for (size_t sent_bytes = 0; sent_bytes < *recv;) {
                if (auto send = co_await net::make_socket_send_awaitable(sock, {buffer + sent_bytes, *recv - sent_bytes}); send) {
                    sent_bytes += *send;
                } else if (!send.error().retryable()) {
                    std::cerr << "Connection crashed while sending with non retryable error code: "
                                << send.error().code << std::endl;
                    co_return;
                }
            }
        } else if (recv && *recv == 0) {
            std::cout << "Connection closed by peer" << std::endl;
            co_return;
        } else if (!recv.error().retryable()) {
            std::cerr << "Connection crashed with non retryable error code: "
                        << recv.error().code << std::endl;
            co_return;
        }
    }
}

int main(int argc, char* argv[]) {
    uint16_t port = std::stoi(argv[1]);
    if (auto server = net::tcp_server_nb::make_server<net::tcp_unspec>(port); server) {
        std::cout << "Starting server at port: " << port << '\n';

        net::connection_pool pool(connection_handler, connection_exception_callback);

        auto server_acceptor = net::server_acceptor(std::move(*server), server_exception_callback);
        for (auto it = server_acceptor.begin(); it != server_acceptor.end();) {
            auto sock = it.consume_and_increment();
            if (sock) {
                std::cout << "Connected to " << sock -> second.first << ":" << sock -> second.second << std::endl;
                pool.add_connection(std::move(sock -> first));
            } else {
                pool.serve_connections();
            }
        }
    } else {
        // todo: visit the error variant properly here to print out detailed error
        std::cerr << "Couldn't create server due to error\n";
    }
}
