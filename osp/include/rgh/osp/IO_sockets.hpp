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

class IPv4_UDP_rogue_client {
public:
    IPv4_UDP_rogue_client( void ) = default;

_RGH_PROTECTED:
    struct _tbl_key_t {
        ipv4_endpoint_t   endpoint   = {};

    //# For the table map, would have been nice to <=> but ipv4_endpoint_t is in BRP, so no STL :(.
        bool operator < ( const _tbl_key_t& rhs_ ) const noexcept { return endpoint < rhs_.endpoint; }
    };
    struct _tbl_value_t {
        _tbl_value_t( void ) = default;
        _tbl_value_t( const _tbl_value_t& ) = delete;

        ~_tbl_value_t( void ) { this->reset(); }

        int           fdesc     = -1;
        sockaddr_in   addr_in   = {};

        void reset( void ) {
            if( fdesc >= 0 ) ::close( std::exchange( fdesc, 0 ) );
            addr_in = {};
        }
    };

_RGH_PROTECTED:
    std::map< _tbl_key_t, _tbl_value_t >   _tbl;

_RGH_PROTECTED:
    [[nodiscard]] const _tbl_value_t* _pull_endpoint( ipv4_endpoint_t endp_ ) {
        _rgh_try {
            auto& end_val = _tbl[ _tbl_key_t{ .endpoint = endp_ } ];

            if( end_val.fdesc < 0 ) {
                memset( &end_val.addr_in, 0, sizeof( sockaddr_in ) );

                end_val.addr_in.sin_family      = AF_INET;
                end_val.addr_in.sin_addr.s_addr = ::htonl( endp_.addr ); 
                end_val.addr_in.sin_port        = ::htons( endp_.port ); 

                end_val.fdesc = socket( AF_INET, SOCK_DGRAM, 0 );
                RGH_ASSERT_OR( end_val.fdesc > 0 ) {
                    RGH_BRDG_LOGE( "ipv4 udp rc: bad fd: {}.", end_val.fdesc );
                    return nullptr;
                }

                RGH_ASSERT_OR( 0 == ::connect( end_val.fdesc, ( sockaddr* )&end_val.addr_in, sizeof( sockaddr_in ) ) ) {
                    RGH_BRDG_LOGE( "ipv4 udp rc: bad connect to {}:{}: {}.", ipv4_addr_str_t{ endp_.addr }.c_str(), endp_.port, std::strerror( errno ) );
                    return nullptr;
                }

                RGH_BRDG_LOGI( "ipv4 udp rc: new endpoint {}:{} ({}).", ipv4_addr_str_t{ endp_.addr }.c_str(), endp_.port, end_val.fdesc );
            }

            return &end_val;
        } _rgh_catch( {} )
        return nullptr;
    }

public:
    ret_t read_from( ipv4_endpoint_t endp_, const port_R_desc_t& desc_ ) {
        const _tbl_value_t* const end_val = this->_pull_endpoint( endp_ );
        RGH_ASSERT_OR( end_val ) return RGH_ERR_GENERAL;

        int ret = ::recv( end_val->fdesc, desc_.dst_ptr, desc_.dst_n, desc_.flags );
        RGH_ASSERT_OR( ret >= 0 ) return RGH_ERR_SYSCALL;

        desc_.set_bc( ret );
        if( desc_.req_all ) RGH_ASSERT_OR( ret == desc_.dst_n ) return RGH_ERR_CORRUPTED;

        return RGH_OK;
    }

    ret_t write_to( ipv4_endpoint_t endp_, const port_W_desc_t& desc_ ) {
        const _tbl_value_t* const end_val = this->_pull_endpoint( endp_ );
        RGH_ASSERT_OR( end_val ) return RGH_ERR_GENERAL;

        int ret = ::send( end_val->fdesc, desc_.src_ptr, desc_.src_n, desc_.flags );
        RGH_ASSERT_OR( ret >= 0 ) return RGH_ERR_SYSCALL;

        desc_.set_bc( ret );
        if( desc_.req_all ) RGH_ASSERT_OR( ret == desc_.src_n ) return RGH_ERR_CORRUPTED;
        
        return RGH_OK;
    }

public:
    friend class Endpoint_port;

    class Endpoint_port : public Port {
    public:
        Endpoint_port( IPv4_UDP_rogue_client* super_, ipv4_endpoint_t endp_ ) {
            const auto* const end_val = super_->_pull_endpoint( endp_ );
            RGH_ASSERT_OR( end_val ) return;

            _fdesc = end_val->fdesc;
        }

    _RGH_PROTECTED:
        int   _fdesc   = -1;

    public:
        ret_t read( const port_R_desc_t& desc_ ) override {
            int ret = ::recv( _fdesc, desc_.dst_ptr, desc_.dst_n, desc_.flags );
            RGH_ASSERT_OR( ret >= 0 ) return RGH_ERR_SYSCALL;

            desc_.set_bc( ret );
            if( desc_.req_all ) RGH_ASSERT_OR( ret == desc_.dst_n ) return RGH_ERR_CORRUPTED;

            return RGH_OK;
        }

        ret_t write( const port_W_desc_t& desc_ ) override {
            int ret = ::send( _fdesc, desc_.src_ptr, desc_.src_n, desc_.flags );
            RGH_ASSERT_OR( ret >= 0 ) return RGH_ERR_SYSCALL;

            desc_.set_bc( ret );
            if( desc_.req_all ) RGH_ASSERT_OR( ret == desc_.src_n ) return RGH_ERR_CORRUPTED;
            
            return RGH_OK;
        }
    };

    Endpoint_port port_of( ipv4_endpoint_t endp_ ) { return { this, endp_ }; }
};

}



