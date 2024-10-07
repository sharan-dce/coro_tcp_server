# coro_tcp_server
A C++-20 coroutines based single threaded tcp server library that can listen for and accept, and also serve multiple connections by switching. As an example, a tcp_echo_server is provided that just returns what a connection sends, and is entirely single threaded (switching happens through `co_await`s).
But you can write your own tcp server using this repo by simply providing your own way of handling a connection in a coroutine, and have the library do the rest of the lifting.

There are 2 main classes/methods in this repo that help you write a generic tcp server that switches between serving connections (also generic - you can pass in any coroutine handler for this), and listening for and accepting new connections, all in 1 single thread. These 2 are net::server_acceptor and net::connection_pool.

I'd like to add that this was more of an exercise/demonstration to show/understand the power of C++20 coroutines, and helped things click for me.

There are some tcp mechanics written in the `/tcp` repo with RAII guards in place, basically abstracting access to non-blocking tcp functionality on macOS (might work on linux too, haven't tested!). The non blocking sockets are required so that our thread does not wait and instead switches to other work it can do in the meantime.

Directions to build:
- clone the repo and `cd` into it: `git clone https://github.com/sharan-dce/coro_tcp_server.git && cd coro_tcp_server`
- `mkdir build && cd build`
- `cmake ../`
- `make`

To run and try out `tcp_echo_server`:
- in the `build` directory we just created and built into, `./echo_server <pick a port for instance 8080`
- you can now form multiple connections to this single threaded server, for instance from 2 or more windows try doing `ncat localhost 8080` and send data! The single threaded echo_server will return it to you! :) - `co_await`ing is powerful!
