#pragma once 

#include "types.h"
#include "stse/stse.hpp"

namespace nasdaq {

struct HeaderView {
    MessageType  message_type;
    LocateCode   stock_locate;
    [[=stse::ignore]] TrackingNum  tracking_number;
    [[=stse::ignore]] Timestamp    timestamp;
};

struct AddOrderMessage {
    static constexpr MessageType MESSAGE_TYPE = 'A';
    ReferenceNum order_reference_number;
    TradeSide    buy_sell_indicator; // 'B' = Buy, 'S' = Sell
    NumShares4   shares;
    Stock        stock;
    Price4       price;
};

struct AddOrderMPIDMessage {
    static constexpr MessageType MESSAGE_TYPE = 'F';
    ReferenceNum order_reference_number;
    TradeSide    buy_sell_indicator; // 'B' = Buy, 'S' = Sell
    NumShares4   shares;
    Stock        stock;
    Price4       price;
    [[=stse::ignore]] MPID         attribution; 
};

struct OrderExecutedMessage {
    static constexpr MessageType MESSAGE_TYPE = 'E';
    ReferenceNum order_reference_number;
    NumShares4   executed_shares;
    [[=stse::ignore]] ReferenceNum match_number;
};

struct OrderExecutedWithPriceMessage {
    static constexpr MessageType MESSAGE_TYPE = 'C';
    ReferenceNum order_reference_number;
    NumShares4   executed_shares;
    [[=stse::ignore]] ReferenceNum match_number;
    [[=stse::ignore]] char         printable;
    [[=stse::ignore]] Price4       execution_price;
};

struct OrderCancelMessage {
    static constexpr MessageType MESSAGE_TYPE = 'X';
    ReferenceNum order_reference_number;
    NumShares4   cancelled_shares;
};

struct OrderDeleteMessage {
    static constexpr MessageType MESSAGE_TYPE = 'D';
    ReferenceNum order_reference_number;
};

struct OrderReplaceMessage {
    static constexpr MessageType MESSAGE_TYPE = 'U';
    ReferenceNum original_order_reference_number;
    ReferenceNum new_order_reference_number;
    NumShares4   shares;
    Price4       price;
};

}
