#include <iostream>
#include <asio.hpp>

//#include <asio/co_spawn.hpp>
//#include <asio/ip/tcp.hpp>

using asio::io_context;
using asio::awaitable;
using asio::co_spawn;
using asio::ip::tcp;
using asio::use_awaitable;
using asio::detached;
namespace this_coro = asio::this_coro;

using namespace std;

awaitable<void> timer_test()
{
    auto executor = co_await this_coro::executor;

    for (int i = 0; i < 10; i++)
    {
        cout << "Waiting event " << i << endl;

        asio::steady_timer t(executor, 2s);

        co_await t.async_wait(asio::use_awaitable);
    }
}

awaitable<void> receiver()
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

        co_spawn(ctx, timer_test(), detached);

        co_spawn(ctx, receiver(), detached);

        ctx.run();
    }
    catch (exception& e)
    {
        cout << "Exception: " << e.what() << endl;
        return -1;
    }


    return 0;
}
