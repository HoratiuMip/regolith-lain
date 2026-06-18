#pragma once
/**
 * @file: ucp/esp32-5x/HRPT_array.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/ucp/core.hpp>

namespace rgh::esp32 {

template< uint8_t _N_ > class HRPT_array {
public:
    HRPT_array( std::array< std::chrono::microseconds, _N_ > cst_ ) {
        for( uint8_t n = 0; n < _N_; ++n ) {
            _arr[ n ].first  = cst_[ n ].count();
            _arr[ n ].second = 0x0; 
        }
    }

_RGH_PROTECTED: 
    std::pair< int64_t, int64_t >   _arr[ _N_ ];

public:
    RGH_inline void disarm(
        void
    ) {
        const int64_t now = esp_timer_get_time();
        for( auto& t : _arr ) if( now - t.second >= t.first ) t.second = 0x0;
    }

    RGH_inline void arm(
        void
    ) {
        const int64_t now = esp_timer_get_time();
        for( auto& t : _arr ) if( t.second == 0x0 ) t.second = now;
    }

public:
    RGH_inline bool timeout( uint8_t n_ ) const { return _arr[ n_ ].second == 0x0; }
    RGH_inline bool operator () ( uint8_t n_ ) const { return this->timeout( n_ ); }

};

#define RGH_ESP32_IF_HRPT_OUT_OR_RTA_MOD( rta_, hk_, arr_, cls_ ) \
    if( auto rtap = (rta_).pull( (hk_), (arr_).timeout((cls_)) ); rtap.has_value() ) if( auto [ rtav, rtam ] = rtap.value(); rtam )

};