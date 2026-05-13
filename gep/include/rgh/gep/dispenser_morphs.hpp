#pragma once
/**
 * @file: gep/dispenser_morphs.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/gep/core.hpp>

namespace rgh {

template< typename _D_, bool _IS_CONTROL_ > struct _dispenser_acquire_base;

template< typename _T_ > class Dispenser_base {
public:
    template< typename, bool > friend struct _dispenser_acquire_base;

public:
    using dispensed_t = _T_;
    using watch_t     = _dispenser_acquire_base< Dispenser_base< _T_ >, false >;
    using control_t   = _dispenser_acquire_base< Dispenser_base< _T_ >, true >;

public:
    virtual _dispenser_acquire_base< Dispenser_base< _T_ >, false > watch( void )  = 0; 
    virtual _dispenser_acquire_base< Dispenser_base< _T_ >, true > control( void ) = 0; 

public:
    virtual HVec< _T_ > hold( void ) = 0;
    virtual _T_* get( void ) = 0;

    RGH_inline _T_* operator -> ( void ) { return this->get(); }
    RGH_inline _T_& operator * ( void ) { return *this->get(); }

};

template< typename _D_, bool _IS_CONTROL_ > struct _dispenser_acquire_base {
public:
    _dispenser_acquire_base( _D_& disp_ ) : _disp{ &disp_ } {}

    _dispenser_acquire_base( const _dispenser_acquire_base& ) = delete;
    _dispenser_acquire_base( _dispenser_acquire_base&& other_ ) : _disp{ std::exchange( other_._disp, nullptr ) } {}

    ~_dispenser_acquire_base( void ) {
        this->release();
    }

public:
    virtual void release( void ) = 0;

public:
    RGH_inline void commit( void ) { return this->release(); }

_RGH_PROTECTED:
    _D_*   _disp   = nullptr;
    
public:
    virtual _D_::dispensed_t* get( void ) = 0;

public:
    RGH_inline _D_::dispensed_t* operator -> ( void ) { return this->get(); }
    RGH_inline _D_::dispensed_t& operator * ( void ) { return *this->get(); }

public:
    virtual bool acquired( void ) const = 0;

    virtual operator bool ( void ) const {
        return _disp != nullptr && this->acquired();
    }

};

}

namespace rgh {

template< typename _T_ > class Dispenser_drop_interval : public Dispenser_base< _T_ > {
public:
    template< typename, bool > friend struct _dispenser_acquire_drop_interval;

public:
    using dispensed_t = _T_;
    using watch_t     = _dispenser_acquire_drop_interval< Dispenser_base< _T_ >, false >;
    using control_t   = _dispenser_acquire_drop_interval< Dispenser_base< _T_ >, true >;

public:
    virtual _dispenser_acquire_base< Dispenser_base< _T_ >, false > watch( void ) override; 
    virtual _dispenser_acquire_base< Dispenser_base< _T_ >, true > control( void ) override;

public:
    virtual HVec< _T_ > hold( void ) override { return _block; }
    virtual _T_* get( void ) override { return _block.get(); }

    RGH_inline _T_* operator -> ( void ) { return this->get(); }
    RGH_inline _T_& operator * ( void ) { return *this->get(); }

_RGH_PROTECTED:
    HVec< _T_ >                        _block;
    std::condition_variable            _cv;
    std::mutex                         _mtx;        
    int64_t                            _born;

public:
    RGH_inline int64_t clock( void ) const {
        using namespace std::chrono;
        return duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
    }

};

template< typename _D_, bool _IS_CONTROL_ > struct _dispenser_acquire_drop_interval {
public:
    template< typename... _VARGS_ >
    _dispenser_acquire_drop_interval( _D_& disp_, _VARGS_&&... vargs_ ) requires _IS_CONTROL_ 
    : _dispenser_acquire_base{ &disp_ }, _block{ HVec< _T_ >::make( std::forward< _VARGS_ >( vargs_ )... ) } {}

    _dispenser_acquire_drop_interval( _D_& disp_, int64_t max_age_, int64_t wait_for_ ) requires not _IS_CONTROL_
    : _dispenser_acquire_base{ &disp_ } {
        std::unique_lock lck{ _disp->_mtx };
        if( _disp->clock() - _disp->_born <= max_age_ ) { _block = _disp->_block; return; }
        
        RGH_ASSERT_OR( std::cv_status::no_timeout == _disp->_cv.wait_for( lck, std::chrono::milliseconds{ wait_for_ } ) ) break;
        _block = _disp->_block;
    }

    _dispenser_acquire_drop_interval( const _dispenser_acquire_drop_interval& ) = delete;

public:
    virtual void release( void ) override {}

    virtual _D_::dispensed_t* get( void ) { return _block.get(); }

    virtual bool acquired( void ) const { return _block; }

_RGH_PROTECTED:
    HVec< _D_::dispensed_t >   _block   = nullptr;
    
};

template< typename _T_ > _dispenser_acquire_drop_interval< Dispenser_drop_interval< _T_ >, false > Dispenser_drop_interval< _T_ >::watch( void ) { return { *this }; }
template< typename _T_ > _dispenser_acquire_drop_interval< Dispenser_drop_interval< _T_ >, true > Dispenser_drop_interval< _T_ >::control( void ) { return { *this }; }

template< typename _T_ > using Dispenser_di = Dispenser_drop_interval< _T_ >; 

}