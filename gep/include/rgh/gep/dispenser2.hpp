#pragma once /*
# FILE: gep/dispenser2.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Next iteration of the Dispenser, using a more scalable handle system.
*/
#include <rgh/gep/core.hpp>

namespace rgh {

#define _rgh_DISP2_HANDLE(hdl_) template<typename _T_> struct DISP2HDL_##hdl_

#define _rgh_DISP2HDL_begin_watch inline void _begin_watch( [[maybe_unused]]Dispenser2<_T_>& _disp
#define _rgh_DISP2HDL_end_watch inline void _end_watch( [[maybe_unused]]Dispenser2<_T_>& _disp
#define _rgh_DISP2HDL_begin_control inline void _begin_control( [[maybe_unused]]Dispenser2<_T_>& _disp
#define _rgh_DISP2HDL_end_control inline void _end_control( [[maybe_unused]]Dispenser2<_T_>& _disp
#define _rgh_DISP2HDL_revert_control inline void _revert_control( [[maybe_unused]]Dispenser2<_T_>& _disp
#define _rgh_DISP2HDL_get_ref inline _T_* _get_ref( [[maybe_unused]]Dispenser2<_T_>& _disp

template< typename _T_ > class Dispenser2;

_rgh_DISP2_HANDLE( MUTEX ) {
    std::shared_mutex   _mtx;

    DISP2HDL_MUTEX() {}
    DISP2HDL_MUTEX( DISP2HDL_MUTEX&& ) noexcept {}

    _rgh_DISP2HDL_begin_watch) { _mtx.lock_shared(); }
    _rgh_DISP2HDL_end_watch) { _mtx.unlock_shared(); }

    _rgh_DISP2HDL_begin_control) { _mtx.lock(); }
    _rgh_DISP2HDL_end_control) { _mtx.unlock(); }
    _rgh_DISP2HDL_revert_control) {}

    _rgh_DISP2HDL_get_ref) { return _disp._hook.get(); }
};
_rgh_DISP2_HANDLE( DROP ) {
    std::shared_ptr<_T_>   _ref;

    _rgh_DISP2HDL_begin_watch) { _ref = _disp._hook; }
    _rgh_DISP2HDL_end_watch) { _ref.reset(); }

    template< typename ..._VArgs_ > _rgh_DISP2HDL_begin_control, _VArgs_&&... vargs_ ) { 
        _ref = std::make_shared<_T_>( std::forward<_VArgs_>( vargs_ )... );
    }
    _rgh_DISP2HDL_end_control) { _disp._hook = std::move( _ref ); }
    _rgh_DISP2HDL_revert_control) { _ref.reset(); }

    _rgh_DISP2HDL_get_ref) { return _ref.get(); }
};

template< typename _T_ > class Dispenser2 {
RGH_protected:
    using _handle_t = std::variant<
        DISP2HDL_MUTEX<_T_>, 
        DISP2HDL_DROP<_T_>
    >;
    friend struct DISP2HDL_MUTEX<_T_>;
    friend struct DISP2HDL_DROP<_T_>;

public:
    template< typename, bool > friend struct _dispenser2_acquire;

public:
    template< template<typename> typename _Hdl_, typename... _VArgs_ > 
    Dispenser2( [[maybe_unused]]_Hdl_<_T_>&&, _VArgs_&&... vargs_ ) 
    : _hook( std::make_shared<_T_>( std::forward< _VArgs_ >( vargs_ )... ) ), 
      _hdl( std::in_place_type<_Hdl_<_T_>> )
    {}

RGH_protected:
    std::shared_ptr<_T_>   _hook   = nullptr;
    _handle_t              _hdl    = std::monostate();

public:
    template< typename ..._VArgs_ > auto watch( _VArgs_&&... args_ ); 
    template< typename ..._VArgs_ > auto control( _VArgs_&&... args_ ); 

};

template< typename _T_, bool _Ctl_ > struct _dispenser2_acquire {
    template< typename ..._VArgs_ > _dispenser2_acquire( Dispenser2<_T_>& disp_, _VArgs_&&... vargs_ )
    : _disp( disp_ ) {
        std::visit( [ &disp_, ...fwdargs_ = std::forward<_VArgs_>( vargs_ ) ]( auto&& hdl_ ) mutable { 
            if constexpr( _Ctl_ )
                hdl_._begin_control( disp_, std::forward<_VArgs_>( fwdargs_ )... );
            else
                hdl_._begin_watch( disp_, std::forward<_VArgs_>( fwdargs_ )... );
        }, disp_._hdl );
    }

    ~_dispenser2_acquire( void ) { this->release(); }
    void commit( void ) requires _Ctl_ { this->release(); }
    void drop( void ) requires _Ctl_ { 
        std::visit( [ this ]( auto&& hdl_ ) mutable { 
            hdl_._revert_control( _disp ); this->release(); 
        }, _disp._hdl );
    }
    void release( void ) {
        std::visit( [ this ]( auto&& hdl_ ) mutable { 
            if constexpr( _Ctl_ ) hdl_._end_control( _disp ); else hdl_._end_watch( _disp ); 
        }, _disp._hdl );
    }

    std::conditional_t< _Ctl_, _T_*, const _T_* > operator -> ( void ) {
        return std::visit( [ this ]( auto&& hdl_ ) mutable { 
            return hdl_._get_ref( _disp );
        }, _disp._hdl );
    }
    auto& operator * ( void ) { return *this->operator->(); }

RGH_protected:
    Dispenser2<_T_>&   _disp;
};

template< typename _T_ > template< typename ..._VArgs_ > 
auto Dispenser2<_T_>::watch( _VArgs_&&... vargs_ ) { return _dispenser2_acquire<_T_,0>{ *this, std::forward< _VArgs_ >( vargs_ )... }; }
template< typename _T_ > template< typename ..._VArgs_ > 
auto Dispenser2<_T_>::control( _VArgs_&&... vargs_ ) { return _dispenser2_acquire<_T_,1>{ *this, std::forward< _VArgs_ >( vargs_ )... }; }

}