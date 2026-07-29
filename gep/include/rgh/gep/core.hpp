#pragma once
/**
 * @file: gep/core.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
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

template<typename _T_>
class HVec {
_RGH_PROTECTED:
    typedef   std::shared_ptr< _T_ >   sptr_t;

public:
    HVec() = default;
    HVec( std::nullptr_t ) : HVec{} {}

    HVec( _T_* ptr_ ) : _sptr{ ptr_ } {}
    HVec( _T_& ref_ ) : _sptr{ &ref_, [](_T_*){} } {}

    HVec( const sptr_t& sp_ ) : _sptr{ sp_ } {}
    HVec( sptr_t&& sp_ ) : _sptr{ std::move( sp_ ) } {}

    HVec( const HVec& ) = default;
    HVec( HVec&& ) = default;

    HVec& operator = ( const HVec& ) = default;
    HVec& operator = ( HVec&& ) = default;
    HVec& operator = ( std::nullptr_t ) { _sptr.reset(); return *this; }

    _T_* get       () const { return _sptr.get(); }
    auto use_count () const { return _sptr.use_count(); }
    void reset     ()       { _sptr.reset(); }

    _T_& operator *  () const { return *_sptr; }
    _T_* operator -> () const { return _sptr.get(); }

    bool operator == ( const _T_* ptr_ ) const { return _sptr.get() == ptr_; }
    bool operator != ( std::nullptr_t )  const { return _sptr != nullptr; }

    explicit operator bool () const { return static_cast< bool >( _sptr ); }

    operator _T_& () const { return *_sptr; }

    template< typename... Args_ >
    static HVec make( Args_&&... args_ ) { return HVec{ std::make_shared< _T_ >( std::forward< Args_ >( args_ )... ) }; }

_RGH_PROTECTED:
    sptr_t   _sptr   = nullptr;
};

template< typename _T_ >
HVec< _T_ > make_hvec( _T_&& t_ ) { return HVec< _T_ >::make( std::move( t_ ) ); }


template< typename _T_ > _T_* rval_addr( _T_&& rval_ ) noexcept { return &rval_; }
#define RGH_RVAL_ADDR( rval_ ) (rgh::rval_addr( (rval_) ) )

template< typename _T_ >
concept C_iterable_char_range = 
    std::convertible_to< _T_, std::string_view >
    ||
    std::ranges::input_range< _T_ > && std::convertible_to<std::ranges::range_value_t< _T_ >, char >;

};