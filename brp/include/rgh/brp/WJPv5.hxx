#pragma once /*
# FILE: brp/WJPv5.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: C++ wrapper over the WJPv5 protocol.
*/

#include "WJPv5.h"

//# If this flag is defined it strongly means that whoever included us linked to rgh::libbrp,
//#   so enable the utilities for it.
#if defined( RGH_BRP )
    #define WJP_ENABLE_RGH
#endif
//# Include the required RGH headers if enabled.
#if defined( WJP_ENABLE_RGH )
    #include "IO_port.hpp"
#endif

namespace wjpxx {

/// Base C++ wrapper over the WJP euclid context.
class Euclid : protected WJP_euclid_context_t {
public:
/// Return type of the reception callback.
    enum class RxCallbackRet : int {
    /// The packet has been parsed successfully. 
        Ok     = WJP_EUCLID_RX_CALLBACK_RET_OK, 
    /// Request a BoF relock. 
        Relock = WJP_EUCLID_RX_CALLBACK_RET_RELOCK
    };

public:
    Euclid( void ) = default;

/// Pass the context and initialize it. The valid state of the context can be
///   checked after with effective().
    Euclid( const WJP_euclid_context_t& ctx_ ) noexcept 
    : WJP_euclid_context_t( ctx_ )
    {}

public:
/// Whether the context has been successfully initialized.
    bool effective( void ) const { return WJP_euclid_effective( this ) == WJP_RET_OK; }
/// Encode bytes into the state machine.
    int push( const WJP_u8* const src_, const WJP_u len_ ) noexcept { return WJP_euclid_push( this, src_, len_ ); }

public:
/// Wrapper for effective().
    operator bool ( void ) const { return this->effective(); }

public:
#if defined( WJP_EUCLID_TEST )
/// Perform a series of self tests for the protocol.
    static auto self_test( void ) noexcept { return WJP_euclid_test(); }
#endif

#if defined( WJP_ENABLE_RGH )
/// Transmit a frame via the given port.
    static rgh::ret_t mov_tx( rgh::io::Port& port_, const WJP_euclid_config_t& cfg_, const WJP_u mrk_, rgh::io::port_W_desc_t desc_ ) {
        WJP_u sz = WJP_euclid_meta_size( &cfg_ ) + desc_.src_n;
        WJP_u8 buf[ sz ];

        WJP_euclid_emplace_frame( &cfg_, buf, mrk_, desc_.src_ptr, desc_.src_n );
        desc_.src_ptr = buf;
        desc_.src_n   = sz;

        return port_.write( desc_ );
    }

    rgh::ret_t mov_tx( rgh::io::Port& port_, const WJP_u mrk_, rgh::io::port_W_desc_t desc_ ) const {
        return Euclid::mov_tx( port_, WJP_euclid_context_t::cfg, mrk_, static_cast<rgh::io::port_W_desc_t&&>( desc_ ) );
    }
#endif

};

}//#wjpxx
