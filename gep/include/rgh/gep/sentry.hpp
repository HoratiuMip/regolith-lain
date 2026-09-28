#pragma once /*
# FILE: gep/sentry.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: The next iteration of the Dispenser.
*/
#include <rgh/gep/core.hpp>

namespace rgh {

template< typename _T_, typename _H_ > struct _sentry_acquire_watch;
template< typename _T_, typename _H_ > struct _sentry_acquire_control;

#define _RGH_SENTRY_HDL( mod_ ) template< typename _T_ > struct sentry_hdl_##mod_
#define _RGH_SENTRY_HDL_MORPH using _MT_

_RGH_SENTRY_HDL( MUTEX ) {
    _RGH_SENTRY_HDL_MORPH = _T_;

    std::shared_mutex   mtx   = {};
};
_RGH_SENTRY_HDL( DROP ) {
    _RGH_SENTRY_HDL_MORPH = std::shared_ptr< _T_ >;

    std::shared_ptr< _T_ >   acq   = nullptr;
};

template< typename _T_, template< typename > typename _H_ >
class Sentry : protected _H_< _T_ > {
public:
    friend struct _sentry_acquire_watch< _T_, _H_< _T_ > >;
    friend struct _sentry_acquire_control< _T_, _H_< _T_ > >;

public:
    Sentry( void ) = default;
    template< typename ..._VARGS_ > Sentry( _VARGS_&&... args_ ) : _obj( args_... ) {}

protected:
    typename _H_< _T_ >::_MT_   _obj   = {};

public:
    template< typename ..._VARGS_ > auto watch( _VARGS_&&... args_ ); 
    template< typename ..._VARGS_ > auto control( _VARGS_&&... args_ ); 
};

template< typename _T_, template< typename > typename _H_ > template< typename ..._VARGS_ > 
auto Sentry< _T_, _H_ >::watch( _VARGS_&&... args_ ) { return _sentry_acquire_watch< _T_, _H_< _T_ > >( *this, std::forward< _VARGS_ >( args_ )... ); }
template< typename _T_, template< typename > typename _H_ > template< typename ..._VARGS_ > 
auto Sentry< _T_, _H_ >::control( _VARGS_&&... args_ ) { return _sentry_acquire_control< _T_, _H_< _T_ > >( *this, std::forward< _VARGS_ >( args_ )... ); }


#define _RGH_SENTRY_ACQ_HEAD( mod_, hdl_ ) \
    template< typename _T_ > struct mod_< _T_, hdl_<_T_> > { \
        static constexpr bool _IS_CONTROL = std::is_same_v< std::remove_cvref_t<mod_>, _sentry_acquire_control<_T_,hdl_<_T_>> >; \
        using _sen_t = Sentry< _T_, hdl_ >; \
        _sen_t* _sen = nullptr; \
        ~mod_( void ) { this->release(); }

#pragma region MUTEX
_RGH_SENTRY_ACQ_HEAD( _sentry_acquire_watch, sentry_hdl_MUTEX )
    _sentry_acquire_watch( _sen_t& sen_ ) : _sen( &sen_ ) { _sen->mtx.lock_shared(); }
    void release( void ) { RGH_ASSERT_AND( _sen ) { _sen->mtx.unlock_shared(); _sen = nullptr; } }
    const _T_* operator -> ( void ) { return &_sen->_obj; }
    const _T_& operator * ( void ) { return _sen->_obj; }
};
_RGH_SENTRY_ACQ_HEAD( _sentry_acquire_control, sentry_hdl_MUTEX )
    _sentry_acquire_control( _sen_t& sen_ ) : _sen( &sen_ ) { _sen->mtx.lock(); }
    void release( void ) { RGH_ASSERT_AND( _sen ) { _sen->mtx.unlock(); _sen = nullptr; } }
    _T_* operator -> ( void ) { return &_sen->_obj; }
    _T_& operator * ( void ) { return _sen->_obj; }
};
#pragma endregion MUTEX

#pragma region DROP
_RGH_SENTRY_ACQ_HEAD( _sentry_acquire_watch, sentry_hdl_DROP )
    _sentry_acquire_watch( _sen_t& sen_ ) : _sen( &sen_ ) { _sen->acq = _sen->obj; }
    void release( void ) { RGH_ASSERT_AND( _sen ) { _sen->acq.reset(); _sen = nullptr; } }
    const _T_* operator -> ( void ) { return _sen->acq.get(); }
    const _T_& operator * ( void ) { return *_sen->acq; }
};
_RGH_SENTRY_ACQ_HEAD( _sentry_acquire_control, sentry_hdl_DROP )
    _sentry_acquire_control( _sen_t& sen_ ) : _sen( &sen_ ) { _sen->acq = _sen->obj; }
    void release( void ) { RGH_ASSERT_AND( _sen ) { if( _sen->acq ) _sen->obj = std::move( _sen->acq ); _sen = nullptr; } }
    _T_* operator -> ( void ) { return _sen->acq.get(); }
    _T_& operator * ( void ) { return *_sen->acq; }

    void drop( void ) { _sen->acq.reset(); this->release(); }
    void commit( void ) { this->release(); }
};
#pragma endregion DROP

}//#namespace rgh