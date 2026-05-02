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
requires std::is_nothrow_copy_constructible_v< _T_ > && 
         std::is_nothrow_move_constructible_v< _T_ >
class Divergent_ring_dynamic_STL {
public:
    typedef   int64_t   clock_t;

_RGH_PROTECTED:
    struct _entry_t {
        clock_t       born;
        HVec< _T_ >   block;
    };

public:
    Divergent_ring_dynamic_STL( clock_t keep_for_ ) noexcept : _keep_for{ keep_for_ } {}

_RGH_PROTECTED:
    clock_t                   _keep_for    = 0x0;
    std::deque< _entry_t >    _queue       = {};
    std::condition_variable   _queue_cv    = {};
    std::mutex                _queue_mtx   = {};

_RGH_PROTECTED:
    void _eject( [[maybe_unused]]const std::unique_lock< decltype( _queue_mtx ) >&, const clock_t clk_now_ ) noexcept {
        for( auto itr = _queue.begin(); itr != _queue.end(); ) {
            if( clk_now_ - itr->born > _keep_for ) {
                itr = _queue.erase( itr );
                continue;
            } else {
                break;
            }
        }
    }

public:
    RGH_inline virtual clock_t clock( void ) const noexcept {
        using namespace std::chrono;
        return duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
    }

public:
    status_t enring( HVec< _T_ > block_ ) noexcept {
        const auto born = this->clock();

        _entry_t entry = {
            .born  = born,
            .block = std::move( block_ )
        };

        std::unique_lock lck{ _queue_mtx };
        this->_eject( lck, born );

        _rgh_try {
            _queue.emplace_back( std::move( entry ) );
        } _rgh_catch( {
            return RGH_ERR_BADALLOC;
        } )

        _queue_cv.notify_all();
        return RGH_OK;
    }

    RGH_inline status_t enring( _T_&& block_ ) noexcept _rgh_try {
        return this->enring( HVec< _T_ >::make( std::move( block_ ) ) );
    } _rgh_catch( {
        return RGH_ERR_BADALLOC;
    } )

    RGH_inline status_t enring( const _T_& block_ ) noexcept _rgh_try {
        return this->enring( HVec< _T_ >::make( block_ ) );
    } _rgh_catch( {
        return RGH_ERR_BADALLOC;
    } )

public:
    HVec< _T_ > dering_far( clock_t max_age_, clock_t wait_for_ ) noexcept {
        std::unique_lock lck{ _queue_mtx };
        if( not _queue.empty() && this->clock() - _queue.back().born <= max_age_ ) return _queue.back().block;

        RGH_ASSERT_OR( std::cv_status::no_timeout == _queue_cv.wait_for( lck, std::chrono::milliseconds{ wait_for_ } ) ) return nullptr;
        RGH_ASSERT_OR( not _queue.empty() ) return nullptr;

        return _queue.back().block;
    }

    _T_ dering_far_or( clock_t max_age_, clock_t wait_for_, _T_ def_ ) noexcept 
    requires( std::is_move_constructible_v< _T_ > and std::is_copy_constructible_v< _T_ > ) {
        auto hv = this->dering_far( max_age_, wait_for_ );
        RGH_ASSERT_OR( hv ) return std::move( def_ );
        return *hv;
    }

public:
    RGH_inline auto size( void ) noexcept {
        std::unique_lock lck{ _queue_mtx };
        return _queue.size();
    }

};

};