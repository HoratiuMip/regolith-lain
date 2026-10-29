#pragma once /*
# FILE: brp/WJPv5.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: C++ wrapper over the WJPv5 protocol.
*/

#include "WJPv5.h"

#if defined( WJP_ENABLE_RGH )
    #include "IO_port.hpp"
#endif

namespace wjpxx {

class Euclid : protected WJP_euclid_context_t {
public:
    enum class RxCallbackRet : int {
        RxCallbackRet_Ok     = WJP_EUCLID_RX_CALLBACK_RET_OK, 
        RxCallbackRet_Relock = WJP_EUCLID_RX_CALLBACK_RET_RELOCK
    };

public:
    Euclid( void ) = default;

    Euclid( const WJP_euclid_context_t& ctx_ ) noexcept 
    : WJP_euclid_context_t( ctx_ )
    {
        WJP_euclid_init( this );
    }

public:
    bool effective( void ) const { return WJP_euclid_effective( this ) == WJP_RET_OK; }
    int push( const WJP_u8* const src_, const WJP_u len_ ) noexcept { return WJP_euclid_push( this, src_, len_ ); }

public:
    operator bool ( void ) const { return this->effective(); }

public:
#if defined( WJP_EUCLID_TEST )
    static auto self_test( void ) noexcept { return WJP_euclid_test(); }
#endif

#if defined( WJP_ENABLE_RGH )
    static rgh::ret_t tx_mov( rgh::IO_port& port_, const WJP_euclid_config_t& cfg_, const WJP_u mrk_, port_W_desc_t desc_ ) {
        WJP_u sz = WJP_euclid_meta_size( cfg_ ) + desc_.src_n;
        WJP_u8 buf[ sz ];

        WJP_euclid_emplace_frame( cfg_, buf, mrk_, desc_.src_ptr, desc_.src_n );
        desc_.src_ptr = buf;
        desc_.src_n   = sz;

        return port_.write( desc_ );
    }
#endif

};

};
