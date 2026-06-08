#include "feed/file_source.h"
#include "feed/feed_handler.h"
#include "spsc/spsc_factory.h"

#include <print>
#include <utility>

int main(int argsc, char** argsv) {
    if (argsc <= 1) {
        std::println("Specify backtest data filepath.");
        return 0;
    }

    auto spsc = make_spsc<std::byte, 1024>();
    
    FileSource  file_source{argsv[1]};
    FeedHandler feed_handler{file_source, std::move(spsc.producer)};

    feed_handler.poll();
    feed_handler.poll();
    feed_handler.poll();
    feed_handler.poll();
    feed_handler.poll();
}
