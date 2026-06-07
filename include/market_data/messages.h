#pragma once 

#include "types.h"

namespace nasdaq {

struct AddOrderMessage {
    static constexpr MessageType MESSAGE_TYPE = 'A';
    LocateCode   stock_locate;
    TrackingNum  tracking_number;
    Timestamp    timestamp;
    ReferenceNum order_reference_number;
    char         buy_sell_indicator; // 'B' = Buy, 'S' = Sell
    NumShares4   shares;
    Stock        stock;
    Price4       price;
};

struct AddOrderMPIDMessage {
    static constexpr MessageType MESSAGE_TYPE = 'F';
    LocateCode   stock_locate;
    TrackingNum  tracking_number;
    Timestamp    timestamp;
    ReferenceNum order_reference_number;
    char         buy_sell_indicator; // 'B' = Buy, 'S' = Sell
    NumShares4   shares;
    Stock        stock;
    Price4       price;
    MPID         attribution; 
};

struct OrderExecutedMessage {
    static constexpr MessageType MESSAGE_TYPE = 'E';
    LocateCode   stock_locate;
    TrackingNum  tracking_number;
    Timestamp    timestamp;
    ReferenceNum order_reference_number;
    NumShares4   executed_shares;
    ReferenceNum match_number;
};

struct OrderExecutedWithPriceMessage {
    static constexpr MessageType MESSAGE_TYPE = 'C';
    LocateCode   stock_locate;
    TrackingNum  tracking_number;
    Timestamp    timestamp;
    ReferenceNum order_reference_number;
    NumShares4   executed_shares;
    ReferenceNum match_number;
    char         printable;
    Price4       execution_price;
};

struct OrderCancelMessage {
    static constexpr MessageType MESSAGE_TYPE = 'X';
    LocateCode   stock_locate;
    TrackingNum  tracking_number;
    Timestamp    timestamp;
    ReferenceNum order_reference_number;
    NumShares4   cancelled_shares;
};

struct OrderDeleteMessage {
    static constexpr MessageType MESSAGE_TYPE = 'D';
    LocateCode   stock_locate;
    TrackingNum  tracking_number;
    Timestamp    timestamp;
    ReferenceNum order_reference_number;
};

struct OrderReplaceMessage {
    static constexpr MessageType MESSAGE_TYPE = 'U';
    LocateCode   stock_locate;
    TrackingNum  tracking_number;
    Timestamp    timestamp;
    ReferenceNum original_order_reference_number;
    ReferenceNum new_order_reference_number;
    NumShares4   shares;
    Price4       price;
};

}
