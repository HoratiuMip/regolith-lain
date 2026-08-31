#pragma once /*
# FILE: brp/IO_utils.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
*/

#include <rgh/brp/descriptor.hpp>
#include <rgh/brp/IO_port.hpp>

namespace rgh { namespace io {

struct wifi_rssi_str_t {
    wifi_rssi_str_t( int32_t rssi_ ) {
        buf[ 0x0 ] = rssi_ >= -90 ? '|' : '.';
        buf[ 0x1 ] = rssi_ >= -80 ? '|' : '.';
        buf[ 0x2 ] = rssi_ >= -70 ? '|' : '.';
        buf[ 0x3 ] = rssi_ >= -60 ? '|' : '.';
        buf[ 0x4 ] = '\0';
    } 

    char   buf[ 5 ];

    RGH_inline operator const char* ( void ) const { return buf; }
};

#pragma region IPv4
//# Maximum size of an IPv4 address string: 4 groups of 3 digits and 3 dots.
constexpr int IPv4_ADDR_STR_MAX_SZ = 4*3 + 3;

//# Check whether the given pointer contains a valid IPv4 address string.
bool ipv4_addr_valid( const char* addr_ );
//# Convert the given string to an IPv4 address.
ipv4_addr_t ipv4_addr_str2n( const char* addr_ );

struct ipv4_addr_str_t {
    ipv4_addr_str_t( void ) = default;
    ipv4_addr_str_t( ipv4_addr_t addr_ ) noexcept;
    ipv4_addr_str_t( const char* addr_ ) noexcept;

    char   buf[ IPv4_ADDR_STR_MAX_SZ+1 ]   = { '\0' }; 

//# Copy "0.0.0.0" into the buffer.
    void make_zero( void ) { strcpy( buf, "0.0.0.0" ); }
//# Invalidate the buffer.
    void make_null( void ) { buf[ 0 ] = { '\0' }; }

    operator bool( void ) const { return buf[ 0x0 ] != '\0'; }
    const char* c_str( void ) const { return buf; }
    operator const char* ( void ) const { return buf; }
};
#pragma endregion IPv4

struct bt_addr_str_t {
    inline static constexpr int   BUF_SIZE   = 0x6*2 + 0x5 + 0x1;

    char   buf[ BUF_SIZE ]   = { '\0' };

    void make_zero( void ) { strcpy( buf, "00:00:00:00:00:00" ); }
    void make_null( void ) { buf[ 0 ] = { '\0' }; }

    static void from_ptr( bt_addr_t addr_, bt_addr_str_t* ptr_, char x_ = 'X' );
    static bt_addr_str_t from( bt_addr_t addr_, char x_ = 'X' );
    static bt_addr_t from( const bt_addr_str_t& addr_str_ );

    char* c_str( void ) { return buf; }
    operator char* ( void ) { return buf; }
};


struct bt_addr_pack_t : bt_addr_t, bt_addr_str_t {
    bt_addr_pack_t( void ) {}

    void pull_str( char x_ = 'X' ) { bt_addr_str_t::from_ptr( *this, this, x_ ); }
    void pull_n( void ) { static_cast< bt_addr_t& >( *this ) = bt_addr_str_t::from( static_cast< bt_addr_str_t& >( *this ) ); }
};


inline constexpr int           NTP_PACKET_SZ   = 48;
inline constexpr uint32_t      NTP_UNIX_OFFSET = 2208988800;
inline constexpr ipv4_port_t   NTP_PORT        = 123;
enum NTP_mode_ : uint8_t {
    NTP_mode_client = 0b011
};
#pragma pack( push, 1 )
struct ntp_packet_t {
    union {
        struct {
            uint8_t   Mode : 3;
            uint8_t   VN   : 3;
            uint8_t   LI   : 2;
        };
        uint8_t B0;
    };
    uint8_t    stratum;
    uint8_t    poll;
    uint8_t    precision;
    uint32_t   root_delay;
    uint32_t   root_dispersion;
    uint32_t   ref_id;
    struct {
        uint32_t   ref_s;
        uint32_t   ref_f;
        uint32_t   org_s;
        uint32_t   org_f;
        uint32_t   rx_s;
        uint32_t   rx_f;
        uint32_t   tx_s; 
        uint32_t   tx_f; 
    }          ts;

    void make_unix( void ) {
        ts.ref_s -= NTP_UNIX_OFFSET;
        ts.ref_f -= NTP_UNIX_OFFSET;
        ts.org_s -= NTP_UNIX_OFFSET;
        ts.org_f -= NTP_UNIX_OFFSET;
        ts.rx_s  -= NTP_UNIX_OFFSET;
        ts.rx_f  -= NTP_UNIX_OFFSET;
        ts.tx_s  -= NTP_UNIX_OFFSET; 
        ts.tx_f  -= NTP_UNIX_OFFSET; 
    }

    static ntp_packet_t client_request( void ) {
        return {
            .Mode = NTP_mode_client,
            .VN   = 4
        };
    }
};
#pragma pack( pop )
static_assert( sizeof( ntp_packet_t ) == NTP_PACKET_SZ );

} };
