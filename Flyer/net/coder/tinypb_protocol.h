#ifndef FLYER_NET_CODER_TINYPB_PROTOCOL_H
#define FLYER_NET_CODER_TINYPB_PROTOCOL_H

#include "Flyer/net/coder/abstract_protocol.h"

namespace Flyer {

struct TinyPBProtocol : public AbstractProtocol {
public:
    TinyPBProtocol() {}
    ~TinyPBProtocol() {}

public:
    static char PB_START;       // 0x02
    static char PB_END;         // 0x03


public:
    int32_t m_pk_len {0};       // package length

    int32_t m_msg_id_len {0};   // length of msg_id
    // msg_id : inherit

    int32_t m_method_name_len {0};   // length of method name
    std::string m_method_name;  // mathod name

    int32_t m_err_code {0};     // 0 means no error
    int32_t m_err_info_len {0}; // length of error info
    std::string m_err_info;     // error info

    std::string m_pb_data;      // serialized data

    int32_t m_check_sum {0};    // checksum

    bool parse_success {false};
};


}

#endif