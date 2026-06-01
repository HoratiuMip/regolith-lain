#pragma once
/**
 * @file: gep/version_atomics.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/gep/core.hpp>

namespace rgh {

struct versioned_atomic_token_t {
public:
    template< typename _T_ > friend class Versioned_atomic;

_RGH_PROTECTED:
    int   _ver   = 0x0;
};

template< typename _T_ >
class Versioned_atomic : public std::atomic< _T_ > {
public:
    using std::atomic< _T_ >::atomic;
    
_RGH_PROTECTED:
    std::atomic_int   _ver   = { 0x0 };

public:
    void push( _T_ val_, std::memory_order mo_ = std::memory_order_seq_cst ) {
        this->store( val_, mo_ );
        _ver.fetch_add( 1, std::memory_order_seq_cst );
    }

    std::optional< _T_ > pull( versioned_atomic_token_t* tok_, std::memory_order mo_ = std::memory_order_seq_cst ) {
        const auto crt_ver = _ver.load( std::memory_order_acquire );
        if( crt_ver == tok_->_ver ) return {};
        
        std::optional< _T_ > ret{ std::in_place, this->load( mo_ ) };
        tok_->_ver = _ver.load( std::memory_order_release );

        return ret;
    }

};

}