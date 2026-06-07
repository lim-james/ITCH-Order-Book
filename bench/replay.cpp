#include "feed/file_source.h"
#include "feed/feed_handler.h"

#include <print>

int main(int argsc, char** argsv) {
    if (argsc <= 1) {
        std::println("Specify backtest data filepath.");
        return 0;
    }
    
    FileSource  file_source{argsv[1]};
    FeedHandler feed_handler{file_source};

    feed_handler.poll();
    feed_handler.poll();
    feed_handler.poll();
    feed_handler.poll();
    feed_handler.poll();
}
