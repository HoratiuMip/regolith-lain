/*
# FILE: osp/IO_inet_utils.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/
#include <rgh/osp/IO_inet_utils.hpp>

#ifdef RGH_TARGET_OS_WINDOWS

#elifdef RGH_TARGET_OS_LINUX
    #include <netdb.h>
    #include <sys/socket.h>
    #include <sys/types.h>
#endif

#include <expected>

namespace rgh::io {

//# A convenient wrapper over getaddrinfo(). See https://man7.org/linux/man-pages/man3/getaddrinfo.3.html.
RGH_IMPL_FNC std::expected< std::vector< ipv4_addr_t >, ret_t > ipv4_hosts_of( std::string_view domain_ ) noexcept {
    RGH_ASSERT_OR( not domain_.empty() ) {
        RGH_BRDG_LOGE( "ipv4 hosts of: no domain provided." );
        return std::unexpected{ RGH_ERR_BADARG };
    }
#if defined( RGH_TARGET_OS_LINUX )
    addrinfo hints{
        .ai_family = AF_INET
    };  

    addrinfo* result;
    RGH_ASSERT_OR_EX( getaddrinfo( domain_.cbegin(), nullptr, &hints, &result ), 0 == ret_ ) {
        RGH_BRDG_LOGE( "ipv4 hosts of: {}", gai_strerror( ret_ ) );
        return std::unexpected{ ret_ };
    }

    std::vector< ipv4_addr_t > hosts;
    for( addrinfo* rp = result; rp != nullptr; rp = rp->ai_next ) {
        const ipv4_addr_t entry = static_cast< ipv4_addr_t >( ntohl( (( sockaddr_in* )( rp->ai_addr ))->sin_addr.s_addr ) );
        RGH_ASSERT_OR( std::ranges::find( hosts, entry ) == hosts.end() ) continue;
        hosts.emplace_back( entry );
    }

    freeaddrinfo( result );

    RGH_ASSERT_OR( not hosts.empty() ) return std::unexpected{ RGH_ERR_NO_RESOLVE };
    return hosts;
#else
    return std::unexpected{ RGH_ERR_NOT_IMPL };
#endif
}

//# Request an NTP packet on the given port.
RGH_IMPL_FNC std::expected< ntp_packet_t, ret_t > ntp_get( Port& port_, bool make_unix_ ) noexcept {
    auto packet = ntp_packet_t::client_request();

    RGH_ASSERT_STATUS_OR( port_.write( { 
        .src_ptr = reinterpret_cast< rgh::byte_t* >( &packet ), 
        .src_n   = sizeof( packet ),
        .req_all = true
    } ) ) return std::unexpected{ status_ };

    RGH_ASSERT_STATUS_OR( port_.read( { 
        .dst_ptr = reinterpret_cast< rgh::byte_t* >( &packet ), 
        .dst_n   = sizeof( packet ),
        .req_all = true
    } ) ) return std::unexpected{ status_ };

#define _RGH_IO_NTP_GET_CVT_END( fld_ ) packet.fld_ = ntohl( packet.fld_ );
    _RGH_IO_NTP_GET_CVT_END( root_delay );
    _RGH_IO_NTP_GET_CVT_END( root_dispersion );
    _RGH_IO_NTP_GET_CVT_END( ref_id );
    _RGH_IO_NTP_GET_CVT_END( ts.ref_s );
    _RGH_IO_NTP_GET_CVT_END( ts.ref_f );
    _RGH_IO_NTP_GET_CVT_END( ts.org_s );
    _RGH_IO_NTP_GET_CVT_END( ts.org_f );
    _RGH_IO_NTP_GET_CVT_END( ts.rx_s );
    _RGH_IO_NTP_GET_CVT_END( ts.rx_f );
    _RGH_IO_NTP_GET_CVT_END( ts.tx_s ); 
    _RGH_IO_NTP_GET_CVT_END( ts.tx_f ); 
#undef _RGH_IO_NTP_GET_CVT_END

    if( make_unix_ ) packet.make_unix();

    return packet;
}

}//#namespace rgh::io