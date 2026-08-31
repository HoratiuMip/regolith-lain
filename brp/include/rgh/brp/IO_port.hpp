#pragma once /*
# FILE: brp/IO_port.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
*/

#include <rgh/brp/descriptor.hpp>

namespace rgh { namespace io {

typedef   uint32_t   ipv4_addr_t;
typedef   uint16_t   ipv4_port_t;

struct ipv4_endpoint_t {
    ipv4_addr_t   addr   = {};
    ipv4_port_t   port   = {};
    
#define _RGH_IO_IPv4_ENDPOINT_COMP_OP( op ) bool operator op ( const ipv4_endpoint_t& rhs_ ) const noexcept { return addr == rhs_.addr ? port op rhs_.port : addr op rhs_.addr; }
    _RGH_IO_IPv4_ENDPOINT_COMP_OP( < )
    _RGH_IO_IPv4_ENDPOINT_COMP_OP( <= )
    _RGH_IO_IPv4_ENDPOINT_COMP_OP( > )
    _RGH_IO_IPv4_ENDPOINT_COMP_OP( >= )
#undef _RGH_IO_IPv4_ENDPOINT_COMP_OP
    bool operator == ( const ipv4_endpoint_t& rhs_ ) const noexcept { return addr == rhs_.addr && port == rhs_.port; }
};

typedef   struct bt_addr_t { uint8_t b[6]; }   bt_addr_t;

typedef   uint8_t    i2c_addr_t;

struct port_R_desc_t {
    byte_t*   dst_ptr      = nullptr;
    int       dst_n        = 0;
    int*      byte_count   = nullptr;
    int       flags        = 0;
    bool      req_all      = false;
    bool      req_time     = false;
    bool      log          = false;

    RGH_inline void set_bc( int bc_ ) const noexcept {
        if( byte_count ) *byte_count = bc_;
    }
};
struct port_W_desc_t {
    byte_t*   src_ptr      = nullptr;
    int       src_n        = 0;
    int*      byte_count   = nullptr;
    int       flags        = 0;
    bool      req_all      = true;
    bool      req_time     = true;
    bool      log          = false;

    RGH_inline void set_bc( int bc_ ) const noexcept {
        if( byte_count ) *byte_count = bc_;
    }
};

class Port {
public:
    virtual status_t read( 
        RGH_IN_OUT   const port_R_desc_t&   desc_ 
    ) = 0;

    virtual status_t write( 
        RGH_IN_OUT   const port_W_desc_t&   desc_ 
    ) = 0;
};


} };
