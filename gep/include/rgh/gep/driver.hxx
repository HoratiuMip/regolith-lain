#pragma once /*
# FILE: gep/driver.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# TLDR:
#   The interface between your program and a device or pretty much anything.
*/
#if defined(__has_include)
    #if __has_include( <rgh/brp/descriptor.hpp> )
        #include <rgh/brp/descriptor.hpp>
    #else
        #include "../../../../brp/descriptor.hpp"
    #endif
#endif 

#include <atomic>
#include <string>
#include <string_view>

namespace rgh::drv {

/// Driver state enum.
/// ---
enum class state_e {
    TERMINATE, //# Terminated drivers will no longer be able to uplink. This is a one way ticket.
    DOWNLINK,  //# Downlinked drivers are considered stopped, in a stale state.
    CONNECT,   //# Connecting drivers are in between downlinked and uplinked.
    UPLINK, 
    DISCONNECT //# I assume you can figure these out.
};

class Interface {
RGH_protected:
    virtual ret_t _driver_uplink( void* ) = 0;
    virtual ret_t _driver_downlink( void* ) = 0;
    virtual ret_t _driver_relink( void* ) { return RGH_ERR_NOT_IMPL; }
    virtual ret_t _driver_uplink_proc( void* ) { return RGH_OK; }
    virtual ret_t _driver_downlink_proc( void* ) { return RGH_OK; }

public:
    virtual std::string_view driver_name( void ) const = 0;
    virtual std::string driver_report( void* ) const { return RGH_NA; }

RGH_protected:
    std::atomic<state_e>   _driver_state   = { state_e::DOWNLINK };

RGH_protected:
    inline void _driver_ss( const state_e s_, std::memory_order mo_ = std::memory_order_relaxed ) {
        _driver_state.store( s_, mo_ );
    }
    inline void _driver_ss_ex( const state_e s_, std::memory_order mo_ = std::memory_order_relaxed ) {
        _driver_state.store( s_, mo_ ); _driver_state.notify_all();
    }

public:
    ret_t driver_uplink( void* va_ ) {
        auto exp = state_e::DOWNLINK;
        RGH_ASSERT_OR( _driver_state.compare_exchange_strong( exp, state_e::CONNECT, std::memory_order_release ) ) {
            return exp == state_e::TERMINATE ? RGH_ERR_TERMINATED : RGH_ERR_WOULD_OVRWR;
        }
      
        RGH_ASSERT_RET_OR( _driver_uplink( va_ ) ) {
            _driver_ss_ex( state_e::DOWNLINK, std::memory_order_release );
            return ret_;
        }
        _driver_ss_ex( state_e::UPLINK, std::memory_order_release );
        (void)_driver_uplink_proc( va_ );
        return RGH_OK;
    }

    ret_t driver_downlink( void* va_ ) {
        auto exp = state_e::UPLINK;
        RGH_ASSERT_OR( _driver_state.compare_exchange_strong( exp, state_e::DISCONNECT, std::memory_order_release ) ) {
            return exp == state_e::TERMINATE ? RGH_ERR_TERMINATED : RGH_ERR_WOULD_OVRWR;
        }

        ret_t ret = _driver_downlink( va_ );
        _driver_ss_ex( State_STOPPED, std::memory_order_release );
        (void)_driver_downlink_proc( va_ );
        return status;
    }

    virtual ret_t driver_relink( void* vad_, void* vau_ ) {
        (void)driver_downlink( vad_ );
        return driver_uplink( vau_ );
    }

};

}//#rgh::drv

