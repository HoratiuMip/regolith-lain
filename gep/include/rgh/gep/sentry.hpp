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

#define _RGH_SENTRY_HDL( mod_ ) struct SENHDL_##mod_
#define _RGH_SENTRY_HDL_MORPH template< typename _T_ > using morph_of_t
#define _RGH_SENTRY_HDL_TOOL template< typename _T_ > struct tool_of_t

_RGH_SENTRY_HDL( MONOSTATE ) {
    _RGH_SENTRY_HDL_MORPH = std::monostate;
    _RGH_SENTRY_HDL_TOOL {};
};
_RGH_SENTRY_HDL( MUTEX ) {
    _RGH_SENTRY_HDL_MORPH = _T_;
    _RGH_SENTRY_HDL_TOOL {
        std::shared_mutex   mtx   = {};
    };
};
_RGH_SENTRY_HDL( DROP ) {
    _RGH_SENTRY_HDL_MORPH = std::shared_ptr< _T_ >;
    _RGH_SENTRY_HDL_TOOL {
        std::shared_ptr< _T_ >   acq   = nullptr;
    };
};

template< typename _T_ = void, typename _H_ = SENHDL_MONOSTATE >
class Sentry : protected _H_ {
public:
    friend struct _sentry_acquire_watch< _T_, _H_ >;
    friend struct _sentry_acquire_control< _T_, _H_ >;

public:
    Sentry( void ) = default;
    template< typename ..._VARGS_ > Sentry( _VARGS_&&... args_ ) : _mrph( args_... ) {}

protected:
    typename _H_::morph_of_t< _T_ >   _mrph   = {};
    typename _H_::tool_of_t< _T_ >    _tool   = {};

public:
    template< typename ..._VARGS_ > auto watch( _VARGS_&&... args_ ); 
    template< typename ..._VARGS_ > auto control( _VARGS_&&... args_ ); 
};

template< typename _T_, typename _H_ > template< typename ..._VARGS_ > 
auto Sentry< _T_, _H_ >::watch( _VARGS_&&... args_ ) { return _sentry_acquire_watch< _T_, _H_ >( *this, std::forward< _VARGS_ >( args_ )... ); }
template< typename _T_, typename _H_ > template< typename ..._VARGS_ > 
auto Sentry< _T_, _H_ >::control( _VARGS_&&... args_ ) { return _sentry_acquire_control< _T_, _H_ >( *this, std::forward< _VARGS_ >( args_ )... ); }


#define _RGH_SENTRY_ACQ_HEAD( mod_, hdl_ ) \
    template< typename _T_ > struct mod_< _T_, hdl_ > { \
        static constexpr bool _IS_CONTROL = std::is_same_v< std::remove_cvref_t<mod_>, _sentry_acquire_control<_T_,hdl_> >; \
        using _sen_t = Sentry< _T_, hdl_ >; \
        _sen_t* _sen = nullptr; \
        ~mod_( void ) { this->release(); }

#pragma region MUTEX
_RGH_SENTRY_ACQ_HEAD( _sentry_acquire_watch, SENHDL_MUTEX )
    _sentry_acquire_watch( _sen_t& sen_ ) : _sen( &sen_ ) { _sen->_tool.mtx.lock_shared(); }
    void release( void ) { RGH_ASSERT_AND( _sen ) { _sen->_tool.mtx.unlock_shared(); _sen = nullptr; } }

    const _T_* operator -> ( void ) { return &_sen->_mrph; }
    const _T_& operator * ( void ) { return _sen->_mrph; }
};
_RGH_SENTRY_ACQ_HEAD( _sentry_acquire_control, SENHDL_MUTEX )
    _sentry_acquire_control( _sen_t& sen_ ) : _sen( &sen_ ) { _sen->_tool.mtx.lock(); }
    void release( void ) { RGH_ASSERT_AND( _sen ) { _sen->_tool.mtx.unlock(); _sen = nullptr; } }

    _T_* operator -> ( void ) { return &_sen->_mrph; }
    _T_& operator * ( void ) { return _sen->_mrph; }

    void drop( void ) { this->release(); }
    void commit( void ) { this->release(); }
};
#pragma endregion MUTEX

#pragma region DROP
_RGH_SENTRY_ACQ_HEAD( _sentry_acquire_watch, SENHDL_DROP )
    _sentry_acquire_watch( _sen_t& sen_ ) : _sen( &sen_ ) { _sen->_tool.acq = _sen->_mrph; }
    void release( void ) { RGH_ASSERT_AND( _sen ) { _sen->_tool.acq.reset(); _sen = nullptr; } }

    const _T_* operator -> ( void ) { return _sen->_tool.acq.get(); }
    const _T_& operator * ( void ) { return *_sen->_tool.acq; }
};
_RGH_SENTRY_ACQ_HEAD( _sentry_acquire_control, SENHDL_DROP )
    _sentry_acquire_control( _sen_t& sen_ ) : _sen( &sen_ ) { _sen->_tool.acq = _sen->_mrph; }
    void release( void ) { RGH_ASSERT_AND( _sen ) { if( _sen->_tool.acq ) _sen->_mrph = std::move( _sen->_tool.acq ); _sen = nullptr; } }

    _T_* operator -> ( void ) { return _sen->_tool.acq.get(); }
    _T_& operator * ( void ) { return *_sen->_tool.acq; }

    void drop( void ) { _sen->_tool.acq.reset(); this->release(); }
    void commit( void ) { this->release(); }
};
#pragma endregion DROP

}//#namespace rgh