#include "util/affinity.h"
#include "spmc/spmc_factory.h"
#include "feed/file_source.h"
#include "feed/feed_handler.h"
#include "market_data/market_data_handler.h"

#include <print>
#include <utility>
#include <thread>
#include <type_traits>

template<auto Stock, std::size_t Consumer_Id, typename spmc_t>
auto create_worker(spmc_t& spmc) {
    return [&] {
        pin_to_core(Consumer_Id + 2);
        OrderBook order_book{};
        using consumer_t = std::remove_reference_t<decltype(spmc.consumers[Consumer_Id])>;
        MarketDataHandler<Stock, consumer_t> market_data_handler{
            std::move(spmc.consumers[Consumer_Id]), std::move(order_book)
        };
        while (market_data_handler.poll());
    };
}

int main(int argsc, char** argsv) {
    if (argsc <= 1) {
        std::println("Specify backtest data filepath.");
        return 0;
    }

    static constexpr std::size_t BUFFER_SIZE = 1 << 20; 
    auto spmc = make_spmc<std::byte, BUFFER_SIZE>(3);

    auto feed_worker = std::jthread([&] {
        pin_to_core(1);
        FileSource  file_source{argsv[1]};
        FeedHandler feed_handler{file_source, std::move(spmc.producer)};
        while (feed_handler.poll());
    });

    auto tsla_worker = std::jthread(create_worker<nasdaq::make_ticker("TSLA"), 0>(spmc));
    auto gold_worker = std::jthread(create_worker<nasdaq::make_ticker("GOLD"), 1>(spmc));
    auto hsbc_worker = std::jthread(create_worker<nasdaq::make_ticker("HSBC"), 2>(spmc));
}
