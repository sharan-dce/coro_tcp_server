# coro_tcp_server
A C++-20 coroutines based single threaded tcp server library that can listen for and accept, and also serve multiple connections by switching. As an example, a tcp_echo_server is provided that just returns what a connection sends, and is entirely single threaded (switching happens through co_awaits).

There are 2 main classes/methods in this repo that help you write a generic tcp server that switches between serving connections (also generic - you can pass in any coroutine handler for this), and listening for and accepting new connections, all in 1 single thread. This was more of an exercise/demonstration to show/understand the power of C++20 coroutines, and helped things click for me.
These 2 are net::server_acceptor and net::connection_pool.

There are some tcp mechanics written in the /tcp repo with RAII guards in place, basically providing access to non-blocking tcp functionality on macOS (might work on linux too, haven't tested!). The non blocking sockets are required so that our thread does not wait and instead switches to other work it can do in the meantime.
