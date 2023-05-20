#include <iostream>
#include <asio.hpp>

#include <asio/co_spawn.hpp>
#include <asio/ip/tcp.hpp>

using asio::io_context;
using asio::awaitable;
using asio::co_spawn;
using asio::ip::tcp;
using asio::use_awaitable;
using asio::detached;
namespace this_coro = asio::this_coro;

using namespace std;

awaitable<void> listener()
{
    auto executor = co_await this_coro::executor;

    for (;;)
    {
        asio::ip::udp::endpoint sender_endpoint;

        constexpr int buffer_size = 64 * 1024;
        char data[buffer_size];

        auto socket = asio::ip::udp::socket(executor, asio::ip::udp::endpoint(asio::ip::udp::v4(), 20777));
        size_t received_count = co_await socket.async_receive_from(asio::buffer(data), sender_endpoint, asio::use_awaitable);

        cout << "received: " << received_count << endl;
    }
}

int main()
{
    try
    {
        cout << "Binding UDP port..." << endl;

        io_context ctx;

        co_spawn(ctx, listener(), detached);

        ctx.run();
    }
    catch (exception& e)
    {
        cout << "Exception: " << e.what() << endl;
        return -1;
    }


    return 0;
}
