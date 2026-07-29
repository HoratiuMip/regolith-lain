#pragma once /*
# FILE: gep/ringatomic.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Atomic lock-free circular ring buffers.
*/
#include <rgh/brp/descriptor.hpp>

namespace rgh {

template< typename T_, size_t CAP_ >
class Ring_atomic_MPSC {
_RGH_PROTECTED:
    std::atomic< size_t >   _head             = { 0x0 };
    std::atomic< size_t >   _tail_push        = { 0x0 };
    std::atomic< size_t >   _tail_pop         = { 0x0 };
    T_                      _buffer[ CAP_ ];

_RGH_PROTECTED:
    RGH_inline static auto _NEXT_ITR( auto idx_ ) { return (idx_ + 1) % CAP_; } 

public:
    status_t push( T_ src_ ) {
        auto tail = _tail_push.load( std::memory_order_relaxed );
        
        while( true ) {
            auto head = _head.load( std::memory_order_acquire );
            RGH_ASSERT_OR( _NEXT_ITR( tail ) != head ) return RGH_ERR_BUSY;

            if( _tail_push.compare_exchange_weak(
                tail, _NEXT_ITR( tail ), 
                std::memory_order_relaxed, std::memory_order_relaxed
            ) ) {
                break;
            }
        }

        _buffer[ tail ] = std::move( src_ );
        _tail_pop.store( _NEXT_ITR( tail ), std::memory_order_release );
        return RGH_OK;
    } 

    status_t pop( T_* dst_ ) {
        auto head = _head.load( std::memory_order_relaxed );
        auto tail = _tail_pop.load( std::memory_order_acquire );

        RGH_ASSERT_OR( head != tail ) return RGH_ERR_DEPLETED;

        *dst_ = std::move( _buffer[ head ] );
        
        _head.store( _NEXT_ITR( head ), std::memory_order_release );
        return RGH_OK;
    }
};

}