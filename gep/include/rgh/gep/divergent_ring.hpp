#pragma once
/**
 * @file: gep/divergent_ring.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/gep/core.hpp>
#include <rgh/gep/dispenser.hpp>

namespace rgh {

template< typename _T_ >
class Divergent_ring_dynamic_STL {
_RGH_PROTECTED:
    struct _entry_t {
        int64_t       born;
        HVec< _T_ >   block;
    };

_RGH_PROTECTED:
    std::deque< _entry_t >    _queue       = {};
    std::condition_variable   _queue_cv    = {};
    std::mutex                _queue_mtx   = {};

public:
    RGH_inline int64_t time_now( void ) const {
        using namespace std::chrono;
        return duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
    }

public:
    status_t enring( HVec< _T_ > block_ ) {
        _entry_t entry = {
            .born  = this->time_now(),
            .block = std::move( block_ )
        };

        std::unique_lock lck{ _queue_mtx };
        _queue.emplace_back( std::move( entry ) );
        _queue_cv.notify_all();

        return RGH_OK;
    }

    RGH_inline status_t enring( _T_&& block_ ) {
        return this->enring( HVec< _T_ >::make( std::move( block_ ) ) );
    } 

    RGH_inline status_t enring( const _T_& block_ ) {
        return this->enring( HVec< _T_ >::make( block_ ) );
    }

public:
    HVec< _T_ > dering( int64_t max_age_, int64_t wait_for_ ) {
        std::unique_lock lck{ _queue_mtx };
        if( _queue.back().born <= max_age_ ) return _queue.back().block();

        RGH_ASSERT_OR( std::cv_status::no_timeout == _queue_cv.wait_for( lck, std::chrono::milliseconds{ wait_for_ } ) ) return nullptr;
        RGH_ASSERT_OR( not _queue.empty() ) return nullptr;

        return _queue.back().block;
    }

};

};