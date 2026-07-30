#pragma once /*
# FILE: gep/core.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Core components of the GE plate.
*/

#include <rgh/brp/descriptor.hpp>

#include <any>
#include <atomic>
#include <charconv>
#include <cmath>
#include <condition_variable>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <shared_mutex>
#include <stack>
#include <string>
#include <string_view>
#include <system_error>
#include <queue>
#include <unordered_map>
#include <utility>

namespace rgh {

/*
# DETAILS: Hyper vectors are reference counting pointers. 
*/
template< typename _T_ > struct hvec_weak_ptr_t { explicit hvec_weak_ptr_t( _T_* ptr_ ) : ptr{ ptr_ } {} _T_* ptr = nullptr; };

template< typename _T_ > class HVec {
_RGH_PROTECTED:
    typedef std::shared_ptr< _T_ > sptr_t;
    template< typename > friend class HVec;

public:
// # General constructors and assign operators.
    HVec() = default;

    HVec( std::nullptr_t ) : HVec{} {}
    HVec& operator = ( std::nullptr_t ) { _sptr.reset(); return *this; }

    HVec( const HVec& ) = default;
    HVec& operator = ( const HVec& ) = default;

    HVec( HVec&& ) noexcept = default;
    HVec& operator = ( HVec&& ) noexcept = default;

// # Niche constructors and assign operators.
    HVec( _T_* ptr_ ) : _sptr{ ptr_ } {}
    HVec& operator = ( _T_* ptr_ ) { _sptr = sptr_t{ ptr_ }; return *this; }

    HVec( _T_& ref_ ) : _sptr{ &ref_, [](_T_*){} } {}
    HVec& operator = ( _T_& ref_ ) { _sptr = sptr_t{ &ref_, [](_T_*){} }; return *this; }

    HVec( hvec_weak_ptr_t< _T_ > hp_  ) : _sptr{ hp_.ptr, [](_T_*){} } {}
    HVec& operator = ( hvec_weak_ptr_t< _T_ > hp_  ) { _sptr = sptr_t{ hp_.ptr, [](_T_*){} }; return *this; }

// # Inheritance chain constructors and assign operators.
    template< typename U_ > requires (std::is_convertible_v< U_*, _T_* >)
    HVec( const HVec< U_ >& other_ ) : _sptr{ other_._sptr } {}
    template< typename U_ > requires (std::is_convertible_v< U_*, _T_* >)
    HVec& operator = ( const HVec< U_ >& other_ ) { _sptr = other_._sptr; return *this; }

    template< typename U_ > requires (std::is_convertible_v< U_*, _T_* >)
    HVec( HVec< U_ >&& other_ ) : _sptr{ std::move( other_._sptr ) } {}
    template< typename U_ > requires (std::is_convertible_v< U_*, _T_* >)
    HVec& operator = ( HVec< U_ >&& other_ ) { _sptr = std::move( other_._sptr ); return *this; }

// # Other constructors and assign operators.
    HVec( const sptr_t& sp_ ) : _sptr{ sp_ } {}
    HVec& operator = ( const sptr_t& sp_  ) { _sptr = sp_; return *this; }

    HVec( sptr_t&& sp_ ) : _sptr{ std::move( sp_ ) } {}
    HVec& operator = ( sptr_t&& sp_  ) noexcept { _sptr = std::move( sp_ ); return *this; }

// # Getter methods.
    _T_* get       () const { return _sptr.get(); }
    auto use_count () const { return _sptr.use_count(); }
    void reset     ()       { _sptr.reset(); }

// # Operators.
    _T_& operator *  () const { return *_sptr; }
    _T_* operator -> () const { return _sptr.get(); }

    bool operator == ( const _T_* ptr_ ) const { return _sptr.get() == ptr_; }
    bool operator != ( std::nullptr_t )  const { return _sptr != nullptr; }

    explicit operator bool () const { return static_cast< bool >( _sptr ); }
    operator _T_& () const { return *_sptr; }

// # Factory.
    template< typename... Args_ >
    static HVec make( Args_&&... args_ ) { return HVec{ std::make_shared< _T_ >( std::forward< Args_ >( args_ )... ) }; }

_RGH_PROTECTED:
    sptr_t   _sptr   = nullptr;
};

/* 
# DETAILS: Trick to inline aggregate structure arguments for C-style functions that expect a pointer to the structure.
*/
template< typename _T_ > _T_* rval_addr( _T_&& rval_ ) noexcept { return &rval_; }
#define RGH_RVAL_ADDR( rval_ ) (rgh::rval_addr( (rval_) ) )

}