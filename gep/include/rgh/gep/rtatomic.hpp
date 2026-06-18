#pragma once
/**
 * @file: gep/rtatomic.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/gep/core.hpp>

namespace rgh {

typedef   int   rt_atomic_ver_t;

template< typename _T_ > class rt_atomic_hook {
public:
    template< typename > friend class rt_atomic;

_RGH_PROTECTED:
    _T_               _prev   = {};
    rt_atomic_ver_t   _ver    = 0x0;
    bool              _m4m    = false;
};

template< typename _T_ > class rt_atomic : public std::atomic< _T_ > {
public:
    typedef   _T_                                        holding_t;
    typedef   std::optional< std::tuple< _T_, bool > >   hook_pull_t;
    typedef   std::function< bool( _T_, _T_ ) >          modch_fnc_t;

public:
    using std::atomic< _T_ >::atomic;
    rt_atomic( void ) {}
    rt_atomic( modch_fnc_t mod_ ) : _mod{ mod_ } {}
    
_RGH_PROTECTED:
    std::atomic< rt_atomic_ver_t >   _ver   = { 0x0 };
    modch_fnc_t                      _mod   = &modch_cmp;

public:
    static RGH_inline rt_atomic_hook< _T_ > get_hook( void ) { return {}; }

public:
    static RGH_inline bool modch_cmp( _T_ next_, _T_ prev_ ) { return next_ != prev_; } 
    template< float TH_ > static RGH_inline bool modch_absdiff( _T_ next_, _T_ prev_ ) { return abs( next_ - prev_ ) >= TH_; } 

public:
    RGH_inline void push( const _T_& next_ ) {
        this->store( next_, std::memory_order_relaxed );
        _ver.fetch_add( 1, std::memory_order_seq_cst );
    }

    hook_pull_t pull( rt_atomic_hook< _T_ >* hk_, const bool fmod_ = false ) {
        hk_->_m4m |= fmod_;

        const auto crt_ver = _ver.load( std::memory_order_acquire );
        if( crt_ver == hk_->_ver ) return std::nullopt;
        hk_->_ver = crt_ver;

        const auto crt_val = this->load( std::memory_order_seq_cst );
        if( hk_->_m4m ) {
            hk_->_m4m = false;
            return std::tuple{ hk_->_prev = crt_val, true };
        }

        const auto crt_mod = this->_mod( crt_val, hk_->_prev );
        if( crt_mod ) hk_->_prev = crt_val;

        return std::tuple{ crt_val, crt_mod };
    }

};

}