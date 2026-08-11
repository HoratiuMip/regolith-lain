#pragma once /*
# FILE: ucp/core.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
*/
#include <rgh/brp/descriptor.hpp>

#include <Arduino.h>

namespace rgh::ino {

#define RGH_INO_NOW_MS_ARG ms_t now_ms_ = millis()

#ifndef RGH_INO_NO_UNSCOPED_DEFINES
    #define INO_NOW_MS_ARG RGH_INO_NOW_MS_ARG
#endif

typedef   uint8_t                pin_t;
typedef   decltype( OUTPUT )     dir_t;
typedef   decltype( HIGH )       lvl_t;
typedef   decltype( millis() )   ms_t;

template< typename... PINS_ > void pin_direction( 
    RGH_IN   const dir_t   dir_, 
    RGH_IN   PINS_...      pins_ 
) {
    ( pinMode( pins_, dir_ ), ... );
}

template< typename... PINS_ > void pin_write(
    RGH_IN   const lvl_t   lvl_,
    RGH_IN   PINS_...      pins_
) {
    ( digitalWrite( pins_, lvl_ ), ... );
}

template< pin_t Pin_, lvl_t NLvl_ > struct input_cc_t {
    input_cc_t( dir_t in_type_ = INPUT_PULLUP ) {
        pin_direction( in_type_, Pin_ );
    }

    bool   actv   = false;
    bool   rise   = false;
    bool   fall   = false;
    bool   _riL   = false;
    bool   _faL   = false;

    void tick( void ) {
        bool read = (digitalRead( Pin_ ) == NLvl_);

        if( read != actv ) {
            rise = read;
            fall = !read;
        } else {
            rise = fall = false;
        }

        _riL |= rise;
        _faL |= fall;
    
        actv = read;
    }

    bool rise_latched( void ) { const bool l = _riL; _riL = false; return l; }
    bool fall_latched( void ) { const bool l = _faL; _faL = false; return l; }

    operator bool( void ) const { return actv; }
};

template< pin_t Pin_, lvl_t NLvl_ > struct timed_output_ccv_t {
    timed_output_ccv_t( 
        RGH_IN   ms_t   ms_ = 1000
    ) {
        pin_direction( OUTPUT, Pin_ );
        this->set_timeout_ms( ms_ );
    }

    void set_timeout_ms( ms_t to_ms_ ) { _to_ms = to_ms_; }
    ms_t elapsed_ms( RGH_INO_NOW_MS_ARG ) { return (now_ms_ - _prev_w_ms); }
    bool expired( RGH_INO_NOW_MS_ARG ) { return this->elapsed_ms( now_ms_ ) >= _to_ms; }
    ms_t remaining_ms( RGH_INO_NOW_MS_ARG ) { return this->expired( now_ms_ ) ? 0 : (_to_ms - (now_ms_ - _prev_w_ms)); }
    lvl_t lvl( void ) { return _lvl; }
    bool active( void ) { return _lvl != NLvl_; }

    void set( RGH_INO_NOW_MS_ARG ) {
        _prev_w_ms = now_ms_;
        pin_write( _lvl = !NLvl_, Pin_ );
    }
    void set_to( ms_t to_ms_, RGH_INO_NOW_MS_ARG ) {
        this->set_timeout_ms( to_ms_ );
        this->set( now_ms_ );
    }
    bool set_once( RGH_INO_NOW_MS_ARG ) {
        RGH_ASSERT_OR( !this->active() ) return false;
        this->set( now_ms_ ); return true;
    }
    void reset( void ) {
        pin_write( _lvl = NLvl_, Pin_ );
    }
    void shadow_set( RGH_INO_NOW_MS_ARG ) {
        _prev_w_ms = now_ms_;
    }
      
    void tick( RGH_INO_NOW_MS_ARG ) {
        RGH_ASSERT_AND( _to_ms == 0 || !this->expired( now_ms_ ) ) return;
        this->reset();
    }

_RGH_PROTECTED:
    ms_t    _prev_w_ms   = 0;
    ms_t    _to_ms       = 0;
    lvl_t   _lvl         = NLvl_;   
};

};