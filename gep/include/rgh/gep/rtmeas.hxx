#pragma once /*
# FILE: gep/rtmeas.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Primarely primitive types with a real-time touch and utility.
*/

namespace rgh::rtcmp {

/// Pushed measurements are updated internally only when the difference between current and previous
///   measurements is greater than the set threshold or the set timeout expired.
/// @tparam meas_t_ The measurement type, e.g. int, float.
/// @tparam time_t_ The time type, e.g. uint32_t.
/// @tparam dmeas_t The type to be compared with the measurements difference.
/// ---
template< typename meas_t_, typename time_t_, typename dmeas_t = meas_t_ > struct timed_delta {
public:
    timed_delta() = default;
    explicit timed_delta( const dmeas_t& dmeas_, const time_t_ dtime_ ) 
    : _dmeas( dmeas_ ), _dtime( dtime_ ) {}

RGH_protected:
    meas_t_   _meas    = {};
    dmeas_t   _dmeas   = {};
    time_t_   _dtime   = {};
    time_t_   _prevt   = {};

public:
    meas_t_& get( void ) { return _meas; }

/// @brief Push a new measurement that took place at a certain time.
/// @param meas_ The measurement value.
/// @param when_ The time when the measurement took place.
/// @return Whether the internal measurement value has changed, i.e. the timeout expired or the delta is above set threshold.
    bool push( const meas_t_& meas_, const time_t_& when_ ) {
        if constexpr( std::is_floating_point_v<meas_t_> ) if( std::isnan( meas_ ) ) return false;
        if( ( when_ - _prevt >= _dtime ) || ( std::abs( _meas - meas_ ) >= _dmeas ) ) {
            _meas  = meas_;
            _prevt = when_;
            return true;
        }
        return false;
    }
};

}//#rgh::rtcmp