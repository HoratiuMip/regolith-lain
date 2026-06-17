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
};

template< typename _T_ > class rt_atomic : public std::atomic< _T_ > {
public:
    typedef   std::function< bool( _T_, _T_ ) >   rt_atomic_mod_t;

public:
    using std::atomic< _T_ >::atomic;
    rt_atomic( void ) {}
    rt_atomic( rt_atomic_mod_t mod_ ) : _mod{ mod_ } {}
    
_RGH_PROTECTED:
    std::atomic< rt_atomic_ver_t >   _ver   = { 0x0 };
    rt_atomic_mod_t                  _mod   = &_cmp_crt_hk_prev;

_RGH_PROTECTED:
    static RGH_inline bool _cmp_crt_hk_prev( _T_ crt_, _T_ hk_prev_ ) { return crt_ != hk_prev_; } 

public:
    RGH_inline void push( const _T_& next_ ) {
        this->store( next_, std::memory_order_relaxed );
        _ver.fetch_add( 1, std::memory_order_seq_cst );
    }

    std::optional< std::tuple< _T_, bool > > pull( rt_atomic_hook< _T_ >& hk_ ) {
        const auto crt_ver = _ver.load( std::memory_order_acquire );
        if( crt_ver == hk_._ver ) return {};
        hk_._ver = crt_ver;

        const auto crt_val = this->load( std::memory_order_seq_cst );    
        const auto crt_mod = this->_mod( crt_val, hk_._val );

        if( crt_mod ) hk_._prev = crt_val;

        return { std::in_place, crt_val, crt_mod };
    }

};

}