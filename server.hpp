#include "coro/task.hpp"
#include "coro/generator.hpp"
#include "tcp/tcp.hpp"
#include "awaitables.hpp"

#include <vector>

namespace net {

// requirement - ConnectionHandler needs to take in the net::tcp_sock object
// as an arg - the resulting coroutine is stored in the connection_handlers
template <typename ConnectionHandler, typename ExceptionCallback>
requires requires(ConnectionHandler handler, net::tcp_sock sock) {
    { handler(std::move(sock)) } -> std::same_as<coro::task<void>>;
}
class connection_pool {
public:
    connection_pool(ConnectionHandler handler_, ExceptionCallback exception_callback_):
        handler{std::move(handler_)},
        exception_callback{std::move(exception_callback_)} {}

    void add_connection(net::tcp_sock socket) {
        auto coroutine = handler(std::move(socket));
        connection_handlers.emplace_back(std::move(coroutine));
    }

    connection_pool(const connection_pool&) = delete;
    connection_pool(connection_pool&&) = default;

    connection_pool& operator=(connection_pool other) {
        swap(other);
        return *this;
    }

    void swap(connection_pool& other) {
        std::swap(handler, other.handler);
        std::swap(connection_handlers, other.connection_handlers);
    }

    void serve_connections() {
        for (size_t index = 0; index < connection_handlers.size();) {
            auto& connection_handler = connection_handlers[index];
            if (connection_handler.done()) {
                std::swap(connection_handler, connection_handlers.back());
                connection_handlers.pop_back();
            } else if (auto exception_ptr = connection_handler.promise().get_safe_exception(); exception_ptr) {
                std::swap(connection_handler, connection_handlers.back());
                connection_handlers.pop_back();
                exception_callback(*exception_ptr);
            } else {
                connection_handler.resume();
                index++;
            }
        }
    }

private:
    ConnectionHandler handler;
    ExceptionCallback exception_callback;
    std::vector<coro::task<void>> connection_handlers;
};

auto make_socket_send_awaitable(net::tcp_sock& sock, std::span<char> data) {
    return coro::awaiter{[&sock, data]() { return sock.send(data); }};
}

auto make_socket_recv_awaitable(net::tcp_sock& sock, std::span<char> buffer) {
    return coro::awaiter{[&sock, buffer]() { return sock.recv(buffer); }};
}

template <typename ExceptionCallback>
std::generator<std::optional<std::pair<tcp_sock, tcp_server_nb::sock_description>>>
server_acceptor(net::tcp_server_nb server_core, ExceptionCallback&& callback) {
    auto connection = server_core.accept();
    for (; connection || connection.error().retryable(); connection = server_core.accept()) {
        co_yield (connection ? 
            std::make_optional<std::pair<tcp_sock, tcp_server_nb::sock_description>>(std::move(*connection)) : std::nullopt);
    }
    std::forward<ExceptionCallback>(callback)(std::move(connection.error()));
}

};
