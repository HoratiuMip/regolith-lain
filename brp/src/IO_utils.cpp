#pragma once /*
# FILE: brp/IO_utils.cpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/

#include <rgh/brp/IO_utils.hpp>

namespace rgh { namespace io {

#pragma region IPv4
RGH_IMPL_FNC bool ipv4_addr_valid( const char* addr_ ) {
    RGH_ASSERT_OR( addr_ ) return false;

    int len = 0, dot = 0, dig = 0;
    for( char c = *addr_; c != '\0' && len < IPv4_ADDR_STR_MAX_SZ; ++len, c = *++addr_ ) {
        if( c == '.' ) {
            if( dig < 1 || dig > 3 ) return false;
            ++dot; dig = 0;
            continue;
        }
        if( c >= '0' && c <= '9' ) {
            if( dig == 2 ) {
                const char* grp = addr_ - 2;
                if( 
                    grp[ 0x0 ] > '2' || 
                    ( grp[ 0x0 ] == '2' && grp[ 0x1 ] > '5' ) ||
                    ( grp[ 0x1 ] == '5' && grp[ 0x2 ] > '5' )
                ) return false;
            }
            if( ++dig > 3 ) return false;
            continue;
        }
        return false;
    }

    return dot == 3 && dig >= 1 && dig <= 3;
}

RGH_IMPL_FNC ipv4_addr_t ipv4_addr_str2n( const char* addr_ ) {
    char buf[ IPv4_ADDR_STR_MAX_SZ ] = { '\0' }; 
    strncpy( buf, addr_, IPv4_ADDR_STR_MAX_SZ );

    ipv4_addr_t result = 0x0;
    char*       head   = buf;

    for( int bi = 0x0; bi < 0x4; ++bi ) {
        char* dot = strchr( head, '.' );
        if( nullptr == dot ) {
            if( 0x3 == bi ) goto l_last;
            else return 0x0;
        }

        *dot = '\0';
    l_last:
        result |= ( ( uint8_t )atoi( head ) ) << ( 0x8*bi ) ; 
 
        head = dot + 1;
    }
 
    return result;
}

RGH_IMPL_FNC ipv4_addr_str_t::ipv4_addr_str_t( ipv4_addr_t addr_ ) noexcept {
    int n = 0x0;
#ifdef RGH_TARGET_END_BIG
    for( int bi = 0x0; bi < 0x4; ++bi )  
#else
    for( int bi = 0x3; bi >= 0x0; --bi )  
#endif
    {
        unsigned char b = reinterpret_cast< unsigned char* >( &addr_ )[ bi ];
        n += snprintf( buf + n, 4, "%u", b );
        *( buf + n ) = '.';
        ++n;
    } 
    buf[ n - 1 ] = buf[ IPv4_ADDR_STR_MAX_SZ ] = '\0';
}
 
RGH_IMPL_FNC ipv4_addr_str_t::ipv4_addr_str_t( const char* addr_ ) noexcept {
    if( ipv4_addr_valid( addr_ ) ) 
        strncpy( buf, addr_, IPv4_ADDR_STR_MAX_SZ );
    else
        this->make_null();
}
#pragma endregion IPv4

RGH_IMPL_FNC void bt_addr_str_t::from_ptr( bt_addr_t addr_, bt_addr_str_t* ptr_, char x_ ) {
    auto* b = addr_.b;
    if( 'X' == x_ ) sprintf( ptr_->buf, "%02X:%02X:%02X:%02X:%02X:%02X", b[0x0], b[0x1], b[0x2], b[0x3], b[0x4], b[0x5] );
    else sprintf( ptr_->buf, "%02x:%02x:%02x:%02x:%02x:%02x", b[0x0], b[0x1], b[0x2], b[0x3], b[0x4], b[0x5] );
}

RGH_IMPL_FNC bt_addr_str_t bt_addr_str_t::from( bt_addr_t addr_, char x_ ) {
    bt_addr_str_t res = {};
    bt_addr_str_t::from_ptr( addr_, &res );
    return res;
}

RGH_IMPL_FNC bt_addr_t bt_addr_str_t::from( const bt_addr_str_t& addr_str_ ) {
    unsigned int b[ 6 ];
    RGH_ASSERT_OR( 6 == sscanf( addr_str_.buf, "%u:%u:%u:%u:%u:%u", b, b+1, b+2, b+3, b+4, b+5 ) ) return { .b = { 0,0,0,0,0,0 } };
    return { .b = { (uint8_t)b[0x0], (uint8_t)b[0x1], (uint8_t)b[0x2], (uint8_t)b[0x3], (uint8_t)b[0x4], (uint8_t)b[0x5] } };
}

} };
