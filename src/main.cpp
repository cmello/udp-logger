#include <iostream>
#include <vector>

#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>

using asio::io_context;
using asio::awaitable;
using asio::co_spawn;
using asio::ip::tcp;
using asio::use_awaitable;
using asio::detached;
using namespace asio::experimental::awaitable_operators;

namespace this_coro = asio::this_coro;

using namespace std;

// waits asynchronously until the specified time elapses.
awaitable<void> async_wait(std::chrono::steady_clock::duration duration)
{
    asio::steady_timer timer(co_await this_coro::executor);
    timer.expires_after(duration);
    co_await timer.async_wait(asio::use_awaitable);
}

// aynchonously buffers data until 2 MB block is filled or caller requests flush explicitly.
// NOTE: couldn't make buffered_write_stream work with stream_file yet, as soon as that works the code below can be removed.
template<class input_buffer_type>
class buffered_file
{

public:
    buffered_file(asio::any_io_executor& executor, const char* path)
        : _file(executor, path, 
            asio::stream_file::create
            | asio::stream_file::read_write
            | asio::stream_file::sync_all_on_write)
    {

    }

    awaitable<void> write_async(input_buffer_type& input_buffer, size_t count)
    {
        // ugly unnecessary memory allocation, until move to asio::buffered_write_stream
        _buffer.insert(_buffer.end(), input_buffer.begin(), input_buffer.begin() + count);

        if (_buffer.size() > _max_buffer_size)
        {
            co_await flush_async();
        }
    }

    awaitable<void> flush_async()
    {
        if (_buffer.empty())
        {
            cout << "nothing to write" << endl;
            co_return;
        }

        cout << "writing " << _buffer.size() << endl;

        co_await asio::async_write(_file, asio::buffer(_buffer), asio::use_awaitable);
        _buffer.clear();
    }

private:
    std::vector<char> _buffer;
    asio::stream_file _file;
    const size_t _max_buffer_size = 2 * 1024 * 1024;
};

// data receive loop.
awaitable<void> receiver()
{
    auto executor = co_await this_coro::executor;

    typedef std::array<char, 64 * 1024> receive_buffer_type;
    receive_buffer_type receive_buffer;
    buffered_file<receive_buffer_type> buffered_file(executor, "udp_traffic.bin");

    auto socket = asio::ip::udp::socket(executor, asio::ip::udp::endpoint(asio::ip::udp::v4(), 20777));

    while(true)
    {
        asio::ip::udp::endpoint sender_endpoint;

        
        // continues after either of the events below happen (|| operator implements that composition of async events)
        auto result = co_await(
            socket.async_receive_from(asio::buffer(receive_buffer), sender_endpoint, asio::use_awaitable) // either received data...
            || async_wait(5s)                                                                             // ... or 2s elapsed with no data
        );

        try
        {

            switch (result.index())
            {
                case 0:
                {
                    // first event happened: received data. add to disk buffer until we have enough to fill a disk write.
                    auto received_count = std::get<0>(result);
                    if (received_count > 0)
                    {
                        //cout << "received: " << received_count << endl;
                        co_await buffered_file.write_async(receive_buffer, received_count);
                    }
                    else
                    {
                        cout << "ERROR: received_count = " << received_count << endl;
                    }
                    break;
                }
                case 1:
                {
                    // second event happened: time elapsed without receiving data. let's flush to disk.
                    cout << "flushing" << endl;
                    co_await buffered_file.flush_async();
                    break;
                }
            }
        }
        catch (exception& e)
        {
            cout << "unhandled Exception: " << e.what() << endl;
        }
    }
}

int main()
{
    try
    {
        cout << "Binding UDP port..." << endl;

        io_context ctx;
        auto work_guard = asio::make_work_guard(ctx); // keeps the executor loop even if there is no more pending work.

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
