#pragma once /*
# FILE: gep/tempo.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Package of time related functions.
*/

#include <rgh/osp/core.hpp>

namespace rgh {

struct ticker_epoch_init_t{};

template< typename _CLK_ > class Ticker_lap {
public:
    Ticker_lap( void ) : _prev_lap{ _CLK_::now() } {}

    Ticker_lap( [[maybe_unused]]ticker_epoch_init_t ) : _prev_lap{} {}

_RGH_PROTECTED:
    _CLK_::time_point   _prev_lap   = {};

public:
    template< typename T_ > T_ peek_lap( void ) const {
        return std::chrono::duration< T_ >( _CLK_::now() - _prev_lap ).count();
    }

    template< typename T_ > T_ lap( void ) {
        const auto now = _CLK_::now();
        return std::chrono::duration< T_ >( now - std::exchange( _prev_lap, now ) ).count();
    }

    template< typename T_ > std::tuple< bool, T_ > cmpxchg_lap( T_ rhs_ ) {
        return (this->peek_lap< T_ >() < rhs_) ? (false, T_{}) : (true, this->lap());
    }

};


// struct int_sleep_t {
// public:

// _RGH_PROTECTED:
//     std::condition_variable   _cv;
//     std::atomic_int           _val;

// public:
//     RGH_inline status_t operator () ( const auto& duration_ ) {
//         std::unique_lock< std::mutex > lock; 
//         return _cv.wait_for( lock, duration_ ) == std::cv_status::no_timeout ? 0x0 : _val.load( std::memory_order_acquire );
//     }

// public:
//     RGH_inline void intr( int val_ = 0x0 ) {
//         if( val_ != 0x0 ) _val.store( val_, std::memory_order_release );
//         _cv.notify_all();
//     }
// };


};
