#pragma once /*
# FILE: gep/WJPv5.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: C++ wrapper over the WJPv5 protocol with STL.
*/

#if defined(__has_include)
    #if __has_include( <rgh/brp/WJPv5.h> )
        #include <rgh/brp/WJPv5.hxx>
    #elif __has_include( "../../../../brp/include/rgh/brp/WJPv5.hxx" )
        #include "../../../../brp/include/rgh/brp/WJPv5.hxx"
    #else
        #include "WJPv5.hxx"
    #endif
#else
    #include "WJPv5.hxx"
#endif 

#include <expected>
#include <functional>
#include <memory>
#include <new>
#include <span>

namespace wjpxx {

template< WJP_u _MLC_Sz_ = 111, WJP_u _BoF_ = WJP_EUCLID_DEFAULT_BOF > 
class Euclid_adapter : public Euclid {
public:
    Euclid_adapter( void ) = default;

    explicit Euclid_adapter( std::size_t rxbufsz_ ) noexcept 
    : Euclid( WJP_EUCLID_CONTEXT_EX( 
            _BoF_, _MLC_Sz_/100%10, _MLC_Sz_/10%10, _MLC_Sz_%10, 
            rxbufsz_ != 0uz ? new (std::nothrow) WJP_u8[ rxbufsz_ ] : nullptr, rxbufsz_, &Euclid_adapter::_callback, static_cast<void*>( this )
      ) ),
      _rxbuf( this->WJP_euclid_context_t::rxbuf )
    {
        WJP_ASSERT_OR( this->Euclid::effective() ) {
            _rxbuf.reset();
            return;
        }
    }

protected:
    std::unique_ptr< WJP_u8[] >   _rxbuf   = nullptr;

protected:
    static int _callback( void* usr_, WJP_u mrk_, WJP_u8* buf_, WJP_u len_ ) {
        return static_cast<int>( static_cast<Euclid_adapter*>( usr_ )->wjpxx_euclid_rx_callback( mrk_, (buf_, len_) ) );
    }

public:
    virtual RxCallbackRet wjpxx_euclid_rx_callback( WJP_u, std::span<WJP_u8> ) = 0;

public:
    std::expected<void, int> push( std::span<const WJP_u8> span_ ) noexcept { 
        const int ret = this->Euclid::push( this, span_.data(), span_.size() );
        WJP_ASSERT_OR( WJP_RET_OK == ret ) return std::unexpected( ret );
        return {};
    }

};

};
