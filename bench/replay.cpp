#include "spsc/spsc_factory.h"
#include "feed/file_source.h"
#include "feed/feed_handler.h"
#include "market_data/market_data_handler.h"

#include <print>
#include <utility>
#include <thread>

int main(int argsc, char** argsv) {
    if (argsc <= 1) {
        std::println("Specify backtest data filepath.");
        return 0;
    }

    auto spsc = make_spsc<std::byte, 1024>();

    auto feed_worker = std::jthread([&] {
        FileSource  file_source{argsv[1]};
        FeedHandler feed_handler{file_source, std::move(spsc.producer)};
        while (feed_handler.poll());
    });

    auto book_worker = std::jthread([&] {
        OrderBook order_book{};
        MarketDataHandler market_data_handler{std::move(spsc.consumer), std::move(order_book)};
        while (market_data_handler.poll());
    });
}
