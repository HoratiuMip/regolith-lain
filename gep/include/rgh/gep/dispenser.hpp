#pragma once
/**
 * @file: gep/dispenser.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/gep/core.hpp>

namespace rgh {

enum DispenserMode_ {
    DispenserMode_Lock, DispenserMode_Trylock,
    DispenserMode_Drop, DispenserMode_DropInterval,
    DispenserMode_Swap, DispenserMode_ReverseSwap,
};

template< typename _T_, bool _IS_CONTROL_ > struct _dispenser_acquire;

enum DispenserFlags_ {
    DispenserFlags_SwapMode_CopyWhenReverseWatchAcquire = ( 1 << 0 )
};

struct dispenser_config_t {
    uint32_t   flags   = 0x0;
};

RGH_inline int64_t dispenser_clock( void ) {
    using namespace std::chrono;
    return duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
}

template< typename _T_ > class Dispenser {
public:
    template< typename, bool > friend struct _dispenser_acquire;

public:
    using dispensed_t = _T_;
    using watch_t     = _dispenser_acquire< _T_, false >;
    using control_t   = _dispenser_acquire< _T_, true >;

public:
    template< typename... _VARGS_ >
    Dispenser( const DispenserMode_ mode_, const dispenser_config_t& config_ = {}, _VARGS_&&... vargs_ ) 
    : _mode{ mode_ }, _config{ config_ }, _M_{ mode_, std::forward< _VARGS_ >( vargs_ )... } 
    {}

    ~Dispenser( void ) {
        switch( _mode ) {
            case DispenserMode_Lock: [[fallthrough]];
            case DispenserMode_Trylock: {
                _M_.lock.~_lock_mode_t();
            break; }
            case DispenserMode_Drop: {
                _M_.drop.~_drop_mode_t();
            break; }
            case DispenserMode_DropInterval: {
                _M_.drop_int.~_drop_int_mode_t();
            break; }
            case DispenserMode_Swap: [[fallthrough]]; 
            case DispenserMode_ReverseSwap: {
                _M_.swap.~_swap_mode_t();
            break; }
        }
    }

_RGH_PROTECTED:
    DispenserMode_       _mode;
    dispenser_config_t   _config;

    union _M_t { 
        template< typename... _VARGS_ >
        _M_t( const DispenserMode_ mode_, _VARGS_&&... vargs_ ) {
            switch( mode_ ) {
                case DispenserMode_Lock: [[fallthrough]];
                case DispenserMode_Trylock:
                    new ( &lock.block ) HVec< _T_ >      { HVec< _T_ >::make( std::forward< _VARGS_ >( vargs_ )... ) };
                    new ( &lock.mtx )   std::shared_mutex{};
                break;
                case DispenserMode_Drop:
                    new ( &drop.block ) HVec< _T_ >{};
                break;
                case DispenserMode_DropInterval: 
                    new ( &drop_int.block ) HVec< _T_ >            {};
                    new ( &drop_int.cv )    std::condition_variable{};
                    new ( &drop_int.mtx )   std::mutex             {};
                    new ( &drop_int.born )  int64_t                { -0x1 };
                break; 
                case DispenserMode_Swap: [[fallthrough]];
                case DispenserMode_ReverseSwap:
                    new ( &swap.blocks[ 0x0 ] ) HVec< _T_ >         { HVec< _T_ >::make( std::forward< _VARGS_ >( vargs_ )... ) };
                    new ( &swap.blocks[ 0x1 ] ) HVec< _T_ >         { HVec< _T_ >::make( std::forward< _VARGS_ >( vargs_ )... ) };
                    new ( &swap.mtxs[ 0x0 ] )   std::shared_mutex   {};
                    new ( &swap.mtxs[ 0x1 ] )   std::shared_mutex   {};
                    new ( &swap.ctl_idx )       std::atomic_uint8_t { 0x0 };
                break;
            }
        } 
        ~_M_t( void ) {}

        struct _lock_mode_t {
            HVec< _T_ >         block;
            std::shared_mutex   mtx;
        } lock;
        struct _drop_mode_t {
            HVec< _T_ >   block;
        } drop;
        struct _drop_int_mode_t {
            HVec< _T_ >                        block;
            std::condition_variable            cv;
            std::mutex                         mtx;        
            int64_t                            born;
        } drop_int;
        struct _swap_mode_t {
            HVec< _T_ >            blocks[ 2 ];
            std::shared_mutex      mtxs[ 2 ];
            std::atomic_uint8_t    ctl_idx;
        } swap;
    } _M_;

public:
    RGH_inline DispenserMode_ switch_swap_mode( DispenserMode_ swap_mode_, std::function< void( dispenser_config_t& ) > modify_config_ ) {
        std::lock_guard lock_1{ _M_.swap.mtxs[ 0x0 ] };
        std::lock_guard lock_2{ _M_.swap.mtxs[ 0x1 ] };
       
        if( modify_config_ ) modify_config_( _config );

        return std::exchange( _mode, swap_mode_ );
    }

public:
    RGH_inline DispenserMode_ mode( void ) const { return _mode; }

    template< typename ..._VARGS_ > RGH_inline _dispenser_acquire< _T_, false > watch( _VARGS_&&... args_ ); 
    template< typename ..._VARGS_ > RGH_inline _dispenser_acquire< _T_, true > control( _VARGS_&&... args_ ); 

public:
    [[nodiscard]] RGH_inline HVec< _T_ > hold( void ) {
        switch( _mode ) {
            case DispenserMode_Lock: [[fallthrough]];
            case DispenserMode_Trylock: return _M_.lock.block;
            case DispenserMode_Drop: return _M_.drop.block;
            case DispenserMode_DropInterval: return _M_.drop_int.block;
            case DispenserMode_Swap: [[fallthrough]];
            case DispenserMode_ReverseSwap: return _M_.swap.blocks[ _M_.swap.ctl_idx.load( std::memory_order_relaxed ) ];
        }
        return nullptr;
    }

    RGH_inline _T_* get( void ) {
        switch( _mode ) {
            case DispenserMode_Lock: [[fallthrough]];
            case DispenserMode_Trylock: return _M_.lock.block.get();
            case DispenserMode_Drop: return _M_.drop.block.get();
            case DispenserMode_DropInterval: return _M_.drop_int.block.get();
            case DispenserMode_Swap: [[fallthrough]];
            case DispenserMode_ReverseSwap: return _M_.swap.blocks[ _M_.swap.ctl_idx.load( std::memory_order_relaxed ) ].get();
        }
        return nullptr;
    }

    RGH_inline _T_* operator -> ( void ) { return this->get(); }
    RGH_inline _T_& operator * ( void ) { return *this->get(); }

};

template< typename _T_, bool _IS_CONTROL_ > struct _dispenser_acquire {
public:
    template< typename ..._VARGS_ >
    [[gnu::hot]] _dispenser_acquire( Dispenser< _T_ >& disp_, _VARGS_&&... vargs_ ) : _disp{ &disp_ }, _M_{ disp_._mode }  {
        switch( _disp->_mode ) {
            case DispenserMode_Lock: {
                if constexpr( _IS_CONTROL_ ) {
                    _disp->_M_.lock.mtx.lock();
                } else {
                    _disp->_M_.lock.mtx.lock_shared();
                }
            break; }
            case DispenserMode_Trylock: {
                if constexpr( _IS_CONTROL_ ) {
                    _disp->_M_.lock.mtx.lock();
                } else {
                    _M_.trylock.acq = _disp->_M_.lock.mtx.try_lock_shared();
                }
            break; }
            case DispenserMode_Drop: {
                if constexpr( _IS_CONTROL_ ) { 
                    _M_.drop.block = HVec< _T_ >::make( std::forward< _VARGS_ >( vargs_ )... );
                } else {
                    _M_.drop.block = _disp->_M_.drop.block;
                }
            break; }
            case DispenserMode_DropInterval: {
                if constexpr( _IS_CONTROL_ ) { 
                    _M_.drop.block = HVec< _T_ >::make( std::forward< _VARGS_ >( vargs_ )... );
                } else if constexpr( sizeof...( vargs_ ) > 0 ) {
                    auto vargs    = std::forward_as_tuple( std::forward< _VARGS_ >( vargs_ )... );
                    auto max_age  = static_cast< int64_t >( std::get< 0x0 >( vargs ) );
                    auto wait_for = static_cast< int64_t >( std::get< 0x1 >( vargs )  );

                    auto& d_M = _disp->_M_.drop_int;
                    
                    std::unique_lock lck{ d_M.mtx };
                    if( dispenser_clock() - d_M.born <= max_age ) { _M_.drop.block = d_M.block; break; }
                    
                    RGH_ASSERT_OR( std::cv_status::no_timeout == d_M.cv.wait_for( lck, std::chrono::milliseconds{ wait_for } ) ) break;
                    _M_.drop.block = d_M.block;
                }
            break; }
            case DispenserMode_Swap: {
                _M_.swap.ctl_idx = _disp->_M_.swap.ctl_idx.load( std::memory_order_acquire ); 
                
                if constexpr( _IS_CONTROL_ ) {
                    _M_.swap.ctl_idx ^= 0x1;
                    _disp->_M_.swap.mtxs[ _M_.swap.ctl_idx ].lock(); 
                } else {
                    _disp->_M_.swap.mtxs[ _M_.swap.ctl_idx ].lock_shared();
                }

                _M_.swap.block = _disp->_M_.swap.blocks[ _M_.swap.ctl_idx ].get();
            break; }
            case DispenserMode_ReverseSwap: {
                if constexpr( _IS_CONTROL_ ) {
                    _M_.swap.ctl_idx = _disp->_M_.swap.ctl_idx.load( std::memory_order_acquire );
                    _disp->_M_.swap.mtxs[ _M_.swap.ctl_idx ].lock();
                } else {
                    if( _disp->_config.flags & DispenserFlags_SwapMode_CopyWhenReverseWatchAcquire ) {
                        auto crt_ctl_idx = _disp->_M_.swap.ctl_idx.load( std::memory_order_acquire );
                        auto sis_ctl_idx = crt_ctl_idx ^ 0x1;
                        std::lock_guard lock_sis{ _disp->_M_.swap.mtxs[ sis_ctl_idx ] };
                        std::lock_guard lock_crt{ _disp->_M_.swap.mtxs[ crt_ctl_idx ] };
                        _disp->_M_.swap.blocks[ sis_ctl_idx ]->operator=( *_disp->_M_.swap.blocks[ crt_ctl_idx ] );
                    }

                    _M_.swap.ctl_idx = _disp->_M_.swap.ctl_idx.fetch_xor( 0x1, std::memory_order_acquire );
                    _disp->_M_.swap.mtxs[ _M_.swap.ctl_idx ].lock_shared();
                }

                _M_.swap.block = _disp->_M_.swap.blocks[ _M_.swap.ctl_idx ].get();
            break; }
        }
    }

    _dispenser_acquire( const _dispenser_acquire& ) = delete;

    _dispenser_acquire( _dispenser_acquire&& other_ )
    : _disp{ std::exchange( other_._disp, nullptr ) }, _M_{ _disp->_mode }
    {
        switch( _disp->_mode ) {
            case DispenserMode_Lock: {
            break; }
            case DispenserMode_Trylock: {
            break; }
            case DispenserMode_Drop: [[fallthrough]];
            case DispenserMode_DropInterval: {
                _M_.drop.block = std::move( other_._M_.drop.block );
            break; }
            case DispenserMode_Swap: [[fallthrough]];
            case DispenserMode_ReverseSwap: {
                _M_.swap.block = std::exchange( other_._M_.swap.block, nullptr );
            break; }
        }
    }

    ~_dispenser_acquire( void ) {
        this->release();
    }

public:
    [[gnu::hot]] void release( void ) {
        if( not _disp ) return;
        switch( _disp->_mode ) {
            case DispenserMode_Lock: {
                if constexpr( _IS_CONTROL_ ) {
                    _disp->_M_.lock.mtx.unlock();
                } else {
                    _disp->_M_.lock.mtx.unlock_shared();
                }
            break; }
            case DispenserMode_Trylock: {
                if constexpr( _IS_CONTROL_ ) {
                    _disp->_M_.lock.mtx.unlock();
                } else {
                    if( std::exchange( _M_.trylock.acq, false ) ) _disp->_M_.lock.mtx.unlock_shared();
                }
            break; }
            case DispenserMode_Drop: {
                if constexpr( _IS_CONTROL_ ) {
                    _disp->_M_.drop.block = std::move( _M_.drop.block );
                } else {
                    _M_.drop.block.reset();
                }
            break; }
            case DispenserMode_DropInterval: {
                if constexpr( _IS_CONTROL_ ) {
                    using namespace std::chrono;

                    std::unique_lock lck{ _disp->_M_.drop_int.mtx };
                    _disp->_M_.drop_int.block = std::move( _M_.drop.block );
                    _disp->_M_.drop_int.born  = dispenser_clock();

                    _disp->_M_.drop_int.cv.notify_all();
                } else {
                    _M_.drop.block.reset();
                }
            break; }
            case DispenserMode_Swap: {
                if constexpr( _IS_CONTROL_ ) {
                    _disp->_M_.swap.ctl_idx.store( _M_.swap.ctl_idx, std::memory_order_release );
                    _disp->_M_.swap.mtxs[ _M_.swap.ctl_idx ].unlock();
                } else {
                    _disp->_M_.swap.mtxs[ _M_.swap.ctl_idx ].unlock_shared();
                }
                _M_.swap.block = nullptr;
            break; }
            case DispenserMode_ReverseSwap: {
                if constexpr( _IS_CONTROL_ ) {
                    _disp->_M_.swap.mtxs[ _M_.swap.ctl_idx ].unlock();
                } else {
                    _disp->_M_.swap.mtxs[ _M_.swap.ctl_idx ].unlock_shared();
                }
                _M_.swap.block = nullptr;
            break; }
        }
        _disp = nullptr;
    }

public:
    RGH_inline void commit( void ) { return this->release(); }

    void drop( void ) {
        switch( _disp->_mode ) {
            case DispenserMode_Drop: [[fallthrough]];
            case DispenserMode_DropInterval: {
                _M_.drop.block.reset();
                _disp = nullptr;
            break; }
        }
    }

_RGH_PROTECTED:
    Dispenser< _T_ >*   _disp;
    
    union _M_t { 
        _M_t( const DispenserMode_ mode_ ) {
            switch( mode_ ) {
                case DispenserMode_Lock: break;
                case DispenserMode_Trylock:
                    new ( &trylock.acq ) bool{ false };
                break;
                case DispenserMode_Drop: [[fallthrough]];
                case DispenserMode_DropInterval:
                    new ( &drop.block ) HVec< _T_ >{};
                break;
                case DispenserMode_Swap: [[fallthrough]];
                case DispenserMode_ReverseSwap:
                    new ( &swap.block )   _T_*   { nullptr };
                    new ( &swap.ctl_idx ) uint8_t{ 0x0 };
                break;
            }
        } 
        ~_M_t( void ) {}

        struct _trylock_mode_t {
            bool   acq;
        } trylock;
        struct _drop_mode_t {
            HVec< _T_ >   block;
        } drop;
        struct _swap_mode_t {
            _T_*      block;
            uint8_t   ctl_idx;
        } swap;
    } _M_;

public:
    RGH_inline _T_* get( void ) {
        switch( _disp->_mode ) {
            case DispenserMode_Lock: [[fallthrough]];
            case DispenserMode_Trylock: return _disp->_M_.lock.block.get();
            case DispenserMode_Drop: [[fallthrough]];
            case DispenserMode_DropInterval: return _M_.drop.block.get();
            case DispenserMode_Swap: [[fallthrough]];
            case DispenserMode_ReverseSwap: return _M_.swap.block;
        }
        return nullptr;
    }

public:
    RGH_inline _T_* operator -> ( void ) { return this->get(); }
    RGH_inline _T_& operator * ( void ) { return *this->get(); }

public:
    RGH_inline operator bool ( void ) const {
        switch( _disp->_mode ) {
            case DispenserMode_Trylock: 
                return _M_.trylock.acq;
            case DispenserMode_Drop: [[fallthrough]];
            case DispenserMode_DropInterval:
                return _M_.drop.block != nullptr;
        }
        return true;
    }

};

template< typename _T_ > template< typename ..._VARGS_ > 
_dispenser_acquire< _T_, false > Dispenser< _T_ >::watch( _VARGS_&&... args_ ) { return _dispenser_acquire< _T_, false >{ *this, std::forward< _VARGS_ >( args_ )... }; }
template< typename _T_ > template< typename ..._VARGS_ > 
_dispenser_acquire< _T_, true > Dispenser< _T_ >::control( _VARGS_&&... args_ ) { return _dispenser_acquire< _T_, true >{ *this, std::forward< _VARGS_ >( args_ )... }; }

template< typename _T_ > using dispenser_watch   = _dispenser_acquire< _T_, false >;
template< typename _T_ > using dispenser_control = _dispenser_acquire< _T_, true >;

}