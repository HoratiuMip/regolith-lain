#pragma once
/**
 * @file: osp/IO_sockets.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/core.hpp>

#include <rgh/brp/IO_port.hpp>
#include <rgh/brp/IO_utils.hpp>


namespace rgh::io { 

class IPv4_socket : public Port {
#ifdef RGH_TARGET_OS_WINDOWS
public:
    inline static constexpr ::SOCKET   INVAL_SOCKFD   = INVALID_SOCKET;
_RGH_PROTECTED:
    ::SOCKET   _fdsock   = INVAL_SOCKFD;
#elifdef RGH_TARGET_OS_LINUX
public:
    inline static constexpr int   INVAL_SOCKFD   = -1;
_RGH_PROTECTED:
    int   _fdsock   = INVAL_SOCKFD;
#endif//# RGH_TARGET_OS

_RGH_PROTECTED:
    struct _link_state_t {
        std::atomic< bool >   alive   = { false };
        ipv4_endpoint_t       endp    = {};
    } _link_state;

public:
    virtual ret_t uplink( const ipv4_endpoint_t& );
    virtual ret_t downlink( void );

public:
    virtual ret_t set_timeout( const port_timeout_desc_t& ) override;
};

class IPv4_TCP_socket : public Port {
#ifdef RGH_TARGET_OS_WINDOWS

_RGH_PROTECTED:
    ::SOCKET   _sock   = INVALID_SOCKET;

#elifdef RGH_TARGET_OS_LINUX

_RGH_PROTECTED:
    int   _sock   = -0x1;

#endif

public:
    struct timeouts_t {
        int   outbound_s   = 0;
        int   inbound_s    = 0;
    };

    struct config_t {
        timeouts_t   timeouts;
    };

public:
    IPv4_TCP_socket( void ) = default;

    IPv4_TCP_socket( const IPv4_TCP_socket& ) = delete;

    IPv4_TCP_socket( IPv4_TCP_socket&& other_ )
    : _sock{ std::exchange( other_._sock, -0x1 ) }, 
      _conn{
        .alive    = { other_._conn.alive.exchange( false, std::memory_order_seq_cst ) },
        .addr_str = std::move( other_._conn.addr_str ),
        .addr     = std::move( other_._conn.addr ),
        .port     = std::move( other_._conn.port )
    } {}

    ~IPv4_TCP_socket( void ) {
        this->disconnect();
    }

_RGH_PROTECTED:
    struct _conn_t {
        std::atomic_bool   alive      = { false };
        ipv4_addr_str_t    addr_str   = {};
        ipv4_addr_t        addr       = 0x0;
        ipv4_port_t        port       = 0x0;
    } _conn;

public:
    RGH_inline const char* addr_c_str( void ) const { return _conn.addr_str.buf; }
    RGH_inline ipv4_port_t port( void ) const { return _conn.port; }

public:
    status_t bind( ipv4_addr_t addr_, ipv4_port_t port_ );
    status_t bind( const char* addr_str_, ipv4_port_t port_ );

public:
    virtual status_t connect( const config_t& config_ );
    virtual status_t disconnect( void );

    virtual status_t listen( void );
    virtual status_t accept( IPv4_TCP_socket* sock_, const config_t& config_  );

public:
    virtual status_t read( const port_R_desc_t& desc_ ) override;
    virtual status_t write( const port_W_desc_t& desc_ ) override;

public:
    status_t timeouts( const timeouts_t& tos_ );

public:
    status_t holding_rx( int* bc_ );

};

}



