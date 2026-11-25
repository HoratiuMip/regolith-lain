/*
# FILE: osp/IO_serial.cpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/
#include <rgh/osp/IO_serial.hpp>

#if (\
!defined( RGH_FLAG_INSIDE_TERMUX ) \
) 
    #define _RGH_SOURCE_IS_COMPILABLE
#endif

#ifdef _RGH_SOURCE_IS_COMPILABLE
#ifdef RGH_TARGET_OS_WINDOWS
#elifdef RGH_TARGET_OS_LINUX
    #include <fcntl.h> 
    #include <poll.h>
    #include <unistd.h>
    #include <termios.h>
    #include <sys/ioctl.h>
#endif

namespace rgh::io {

#pragma region Serial
#ifdef RGH_TARGET_OS_WINDOWS
ret_t Serial::open( const char* device_, const serial_config_t& config_ ) {
    if( _port != SERIAL_INVALID_HANDLE ) this->close();

    std::string device = device_;
    if( not device.starts_with( "\\\\.\\" ) ) device.insert( 0, "\\\\.\\" );
    _port = CreateFileA( device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    RGH_ON_SCOPE_EXIT_L( [ this ] ( void ) -> void {
        if( _port != SERIAL_INVALID_HANDLE ) {
            CloseHandle( std::exchange( _port, SERIAL_INVALID_HANDLE ) );
        }
    } );

    RGH_ASSERT_OR( _port != SERIAL_INVALID_HANDLE ) {
        RGH_LOGE_IO_EX( RGH_ERR_SYSCALL, "Could not open serial port \"{}\".", device );
        return RGH_ERR_SYSCALL;
    }

    RGH_ASSERT_OR( FlushFileBuffers( _port ) ) {
        RGH_LOGW_IO( "Could not flush serial port buffers for opened serial port \"{}\", error code [{}].", device, GetLastError() );
    }

    COMMTIMEOUTS timeouts{
        .ReadIntervalTimeout         = config_.rx_ib_timeout,
        .ReadTotalTimeoutMultiplier  = 0,
        .ReadTotalTimeoutConstant    = config_.rx_fb_timeout,
        .WriteTotalTimeoutMultiplier = 0,
        .WriteTotalTimeoutConstant   = config_.tx_timeout
    };

    RGH_ASSERT_OR( SetCommTimeouts( _port, &timeouts ) ) {
        RGH_LOGW_IO( "Could not set the configured timeouts for serial port \"{}\", error code [{}].", device, GetLastError() );
    }

    DCB state{ 0 }; state.DCBlength = sizeof( DCB );

    RGH_ASSERT_OR( GetCommState( _port, &state ) ) {
        RGH_LOGW_IO( "Could not read the default state of the serial port \"{}\". Some configurations might be defaulted or contain garbage values. Error code [{}].", device, GetLastError() );
    }

    state.BaudRate     = ( uint32_t )config_.baud_rate;
    state.ByteSize     = config_.byte_size;
    state.Parity       = config_.parity;
    state.StopBits     = config_.stopbit;
    state.fOutxCtsFlow = false;  
    state.fRtsControl  = RTS_CONTROL_DISABLE;
    state.fOutX        = false;  
    state.fInX         = false;   
    state.fBinary      = true;

    RGH_ASSERT_OR( SetCommState( _port, &state ) ) {
        RGH_LOGE_IO( "Could not configure the serial port \"{}\", error code [{}].", device, GetLastError() );
        return -0x1;
    }

    RGH_ON_SCOPE_EXIT_DROP;
    _device = std::move( device );
    _config = config_;

    if( config_.purge_on_open ) this->purge();

    RGH_LOGI_IO( "Opened and configured serial port \"{}\" successfully @{}bauds.", _device, _config.baud_rate );
    return 0x0;
}

ret_t Serial::close( void ) {
    if( _port == SERIAL_INVALID_HANDLE ) return 0x0;
    
    if( _config.purge_on_close ) this->purge();
    CloseHandle( std::exchange( _port, INVALID_HANDLE_VALUE ) );
    RGH_LOGI_IO( "Closed serial port \"{}\".", _device );
    _device = "";
    _config = serial_config_t{};
    return 0x0;
}

ret_t Serial::read( const port_R_desc_t& desc_ ) {
    uint32_t byte_count = 0;
    ReadFile( _port, desc_.dst_ptr, desc_.dst_n, ( LPDWORD )&byte_count, nullptr );
    
    if( desc_.byte_count ) *desc_.byte_count = byte_count;

    if( desc_.req_all && byte_count != desc_.dst_n ) {
        return RGH_ERR_FLOW;
    }

    return RGH_OK;
}

ret_t Serial::write( const port_W_desc_t& desc_ ) {
    uint32_t byte_count = 0;
    WriteFile( _port, desc_.src_ptr, desc_.src_n, ( LPDWORD )&byte_count, nullptr );

    if( desc_.byte_count ) *desc_.byte_count = byte_count;

    if( desc_.req_all && byte_count != desc_.src_n ) {
        return -0x1;
    }

    return 0x0;
}

int Serial::rx_q_sz( void ) const {
    COMSTAT stat; memset( &stat, 0x0, sizeof( COMSTAT ) );
    RGH_ASSERT_OR( ClearCommError( _port, nullptr, &stat ) ) return -0x1;
    return stat.cbInQue;
}

ret_t Serial::purge( void ) const {
    RGH_ASSERT_OR( PurgeComm( _port, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR ) ) {
        RGH_LOGW_IO( "Could not purge serial port \"{}\", error code [{}].", _device, GetLastError() );
        return -0x1;
    }
    return 0x0;
}

ret_t Serial::dtr_reset( void ) const {
    return RGH_ERR_NOT_IMPL;
}
ret_t Serial::dtr_set( void ) const {
    return RGH_ERR_NOT_IMPL;
}

#elifdef RGH_TARGET_OS_LINUX

static constexpr speed_t _baud4termios( uint32_t baud_ ) noexcept {
    switch( baud_ ) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
#ifdef B460800
        case 460800: return B460800;
#endif
#ifdef B921600
        case 921600: return B921600;
#endif
        default:     return B9600; 
    }
    RGH_UNREACHABLE;
}

ret_t Serial::open( 
    RGH_IN   const char*              device_, 
    RGH_IN   const serial_config_t&   config_ ) 
{
    RGH_ASSERT_OR( not this->is_connected() ) {
        RGH_ASSERT_STATUS_OR( this->close() ) {
            RGH_BRDG_LOGE( "serial: bad close of {} when opening {}.", _device, device_ );
            return status_;
        }
    }

    RGH_ASSERT_OR( device_ ) { errno = EINVAL; return RGH_ERR_BADARG; }

    _port = ::open( device_, O_RDWR | O_NOCTTY | O_NONBLOCK );
    RGH_ASSERT_OR( _port != SERIAL_INVALID_HANDLE ) {
        RGH_BRDG_LOGE( "serial: bad open on {}.", device_ );
        return RGH_ERR_SYSCALL;
    }

    _device = device_;
    _config = config_;

    struct termios tty; std::memset( &tty, 0x0, sizeof( tty ) );

    RGH_ON_SCOPE_EXIT_L( [ this ] ( void ) -> void {
        ::close( std::exchange( _port, SERIAL_INVALID_HANDLE ) );
    } );

    RGH_ASSERT_OR( tcgetattr( _port, &tty ) == 0x0 ) {
        RGH_BRDG_LOGE( "serial: bad getattr on {}.", _device );
        return RGH_ERR_SYSCALL;
    }

    cfmakeraw( &tty );
    tty.c_cflag &= ~CRTSCTS;

    const speed_t baud = _baud4termios( _config.baud_rate );
    RGH_ASSERT_OR( cfsetospeed( &tty, baud ) == 0x0 and cfsetispeed( &tty, baud ) == 0x0 ) {
        RGH_BRDG_LOGE( "serial: bad baud set on {}.", _device );
        return RGH_ERR_SYSCALL;
    }

    tty.c_cflag &= ~CSIZE;
    if( _config.byte_size == 5 ) {
        tty.c_cflag |= CS5;
    } else if( _config.byte_size == 6 ) {
        tty.c_cflag |= CS6;
    } else if( _config.byte_size == 7 ) {
        tty.c_cflag |= CS7;
    } else {
        tty.c_cflag |= CS8;
    }

    switch( _config.parity ) {
        case SERIAL_PARITY_NONE: tty.c_cflag &= ~PARENB; break;
        case SERIAL_PARITY_ODD: tty.c_cflag |= PARENB; tty.c_cflag |= PARODD; break;
        case SERIAL_PARITY_EVEN: tty.c_cflag |= PARENB; tty.c_cflag &= ~PARODD; break;
        case SERIAL_PARITY_MARK: tty.c_cflag |= PARENB; tty.c_cflag |= PARODD; tty.c_cflag |= CMSPAR; break;
        case SERIAL_PARITY_SPACE: tty.c_cflag |= PARENB; tty.c_cflag &= ~PARODD; tty.c_cflag |= CMSPAR; break;
        default: tty.c_cflag &= ~PARENB;
    }
   
    (void)( _config.stopbit == SERIAL_STOPBIT_ONE ? tty.c_cflag &= ~CSTOPB : tty.c_cflag |= CSTOPB );

    tty.c_cflag |= ( CLOCAL | CREAD );
    tty.c_lflag &= ~( ICANON | ECHO | ECHOE | ISIG );
    tty.c_iflag &= ~( IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL );
    tty.c_oflag &= ~OPOST;

    tty.c_cc[ VMIN ]  = 0;
    tty.c_cc[ VTIME ] = ( cc_t )( _config.rx_fb_timeout / 100 ); if( tty.c_cc[ VTIME ] == 0 ) tty.c_cc[ VTIME ] = 1;

    RGH_ASSERT_OR( tcsetattr( _port, TCSANOW, &tty ) == 0x0 ) {
        RGH_BRDG_LOGE( "serial: bad config of {}.", _device );
        return RGH_ERR_SYSCALL;
    }

    if( _config.dtr_on_open != 0 ) this->dtr_toggle( _config.dtr_on_open );

    if( _config.purge_on_open ) {
        RGH_ASSERT_STATUS_OR( this->purge() ) {
            RGH_BRDG_LOGW( "serial: bad purge when opening {}.", _device );
        }
    }

    RGH_ON_SCOPE_EXIT_DROP;
    RGH_BRDG_LOGI( "serial: opened {}.", _device );
    return RGH_OK;
}

ret_t Serial::close( void ) {
    RGH_ASSERT_OR( this->is_connected() ) return RGH_OK;

    if( _config.purge_on_close ) {
        RGH_ASSERT_STATUS_OR( this->purge() ) {
            RGH_BRDG_LOGW( "serial: bad purge when closing {}.", _device );
        }
    }

    RGH_ASSERT_OR( ::close( _port ) == 0x0 ) {
        RGH_BRDG_LOGE( "serial: bad close on {}.", _device );
        return RGH_ERR_SYSCALL;
    }

    RGH_BRDG_LOGI( "serial: closed {}.", _device );
    _port = SERIAL_INVALID_HANDLE;
    _device.clear();
    return RGH_OK;
}

ret_t Serial::read( const port_R_desc_t& desc_ ) {
    ssize_t bc = ::read( _port, desc_.dst_ptr, desc_.dst_n );

    RGH_ASSERT_OR( bc >= 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK ) { bc = 0; goto l_ok; }
        return RGH_ERR_SYSCALL;
    }

l_ok:
    desc_.set_bc( bc );
    if( desc_.req_all and bc != desc_.dst_n ) return RGH_ERR_PARTIAL;
    
    return RGH_OK;
}

ret_t Serial::write( const port_W_desc_t& desc_ ) {
    ssize_t bc = ::write( _port, desc_.src_ptr, desc_.src_n );

    RGH_ASSERT_OR( bc >= 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK ) { bc = 0; goto l_ok; }
        return RGH_ERR_SYSCALL;
    }

l_ok:
    desc_.set_bc( bc );
    if( desc_.req_all and bc != desc_.src_n ) return RGH_ERR_BUSY;
    
    return RGH_OK;
}

int Serial::rx_q_sz( void ) const {
    int bca = 0;
    RGH_ASSERT_OR( ::ioctl( _port, FIONREAD, &bca ) >= 0x0 ) return 0;
    return bca;
}

ret_t Serial::purge( void ) const {
    RGH_ASSERT_OR( ::tcflush( _port, TCIOFLUSH ) == 0 ) return RGH_ERR_SYSCALL;
    return RGH_OK;
}

ret_t Serial::dtr_reset( void ) const {
    int status = 0x0;
    RGH_ASSERT_OR( 0x0 == ioctl( _port, TIOCMGET, &status ) ) return RGH_ERR_SYSCALL;
    status |= TIOCM_DTR;
    return ioctl( _port, TIOCMSET, &status ) == 0x0 ? RGH_OK : RGH_ERR_SYSCALL;
}
ret_t Serial::dtr_set( void ) const {
    int status = 0x0;
    RGH_ASSERT_OR( 0x0 == ioctl( _port, TIOCMGET, &status ) ) return RGH_ERR_SYSCALL;
    status &= ~TIOCM_DTR;
    return ioctl( _port, TIOCMSET, &status ) == 0x0 ? RGH_OK : RGH_ERR_SYSCALL;
}

#endif
#pragma endregion Serial

#pragma region Fasttrack_serial

#ifdef RGH_TARGET_OS_WINDOWS

#elifdef RGH_TARGET_OS_LINUX

#ifndef RGH_IMPL_SEL_IO_FASTTRACK_SERIAL__POLL_LOOP
#define RGH_IMPL_SEL_IO_FASTTRACK_SERIAL__POLL_LOOP
RGH_IMPL_FNC void Fasttrack_serial::_poll_loop( 
    RGH_IN_OUT   std::stop_token   stop_tok_ 
) noexcept {
    pollfd pfd = {
        .fd     = _port,
        .events = POLLIN
    };

    while( not stop_tok_.stop_requested() ) {
        int plr = ::poll( &pfd, 1, 500 );

        RGH_ASSERT_AND( plr > 0 ) {
            RGH_ASSERT_OR( (pfd.revents & (POLLHUP | POLLERR)) == 0x0 ) {
                RGH_BRDG_LOGE( "ft_serial: sudden disconnect of {}.", _device );
                break;
            }

            RGH_ASSERT_OR( pfd.revents & POLLIN ) continue;

            int buffer_sz = std::min( this->rx_q_sz(), 1024 );
            byte_t buffer[ buffer_sz ];
            RGH_ASSERT_STATUS_OR( this->read( {
                .dst_ptr    = buffer,
                .dst_n      = buffer_sz,
                .byte_count = &buffer_sz
            } ) ) continue;

            RGH_ASSERT_AND( buffer_sz > 0 ) this->_bytes_cb( buffer, buffer_sz );

        } else RGH_ASSERT_AND( plr == 0 ) {
            /*# Timeout on the poll. */
        } else {
            if( errno == EINTR ) continue;
            RGH_BRDG_LOGE( "ft_serial: bad poll on {}: {}.", _device, std::strerror( errno ) );
            break;
        }
    }
    RGH_BRDG_LOGW( "ft_serial: poll loop exited on {}.", _device );
}
#endif//# RGH_IMPL_SEL_IO_FASTTRACK_SERIAL__POLL_LOOP

#endif//# RGH_TARGET_OS

RGH_IMPL_FNC ret_t Fasttrack_serial::open( 
    RGH_IN   const char*              device_, 
    RGH_IN   const serial_config_t&   config_,
    RGH_IN   bytes_cb_t               bytes_cb_
) noexcept {
    RGH_ASSERT_STATUS_OR_RET( this->Serial::open( device_, config_ ) );
    _bytes_cb = std::move( bytes_cb_ );
    _poll_th = std::jthread{ &Fasttrack_serial::_poll_loop, this };
    RGH_BRDG_LOGI( "ft_serial: poll loop entered on {}.", _device );
    return RGH_OK;
}

#ifndef RGH_IMPL_SEL_IO_FASTTRACK_SERIAL__POLL_LOOP
#define RGH_IMPL_SEL_IO_FASTTRACK_SERIAL__POLL_LOOP
RGH_IMPL_FNC void Fasttrack_serial::_poll_loop(
    RGH_IN_OUT   std::stop_token   stkn_
) noexcept {
    while( not stkn_.stop_requested() ) {
        for( int avail; ( avail = this->rx_q_sz() ) > 0; ) {
            byte_t buf[ 0x400 ];

            int buf_eff_len = std::min( avail, static_cast< int >( sizeof( buf ) ) );
            RGH_ASSERT_STATUS_OR( this->read( {
                .dst_ptr    = buf,
                .dst_n      = buf_eff_len,
                .byte_count = &buf_eff_len
            } ) ) break;

            RGH_ASSERT_OR( buf_eff_len > 0 ) break;
            this->_bytes_cb( buf, buf_eff_len );
        }
        std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
    }

    RGH_BRDG_LOGW( "ft_serial: poll loop exited on {}.", _device );
}
#endif//# RGH_IMPL_SEL_IO_FASTTRACK_SERIAL__POLL_LOOP

#pragma endregion Fasttrack_serial

}

namespace rgh::io::v2 {

#pragma region v2::Simple_serial
#ifdef RGH_TARGET_OS_WINDOWS
RGH_IMPL_FNC ret_t Simple_serial::uplink( 
    RGH_IN   serial_config_t   req_cfg_
) noexcept {
//# Validate the requested device path.
    RGH_ASSERT_OR( not req_cfg_.device.empty() ) { 
        RGH_BRDG_LOGE( "serial: empty device path." );
        return RGH_ERR_BADARG; 
    }
//# Assert that this port is not already uplinked. Relink or return based on the current config.
//# The configuration structure pointer must be valid if the handle is valid.
    if( _handle != SERIAL_INVALID_HANDLE ) {
        if( not _config.allow_up_ovrwr ) {
            RGH_BRDG_LOGE( "serial: uplink to {} would override existing link to {}.", req_cfg_.device, _config.device );
            return RGH_ERR_WOULD_OVRWR;
        }
        RGH_ASSERT_RET_OR( this->Simple_serial::downlink() ) {
            RGH_BRDG_LOGE( "serial: bad downlink when uplinking to {}.", req_cfg_.device );
            return ret_;
        }
    }

    serial_config_t cfg = std::move( req_cfg_ );
//# Open through the device namespace so that COM10 and above work. The stored config keeps the requested path.
    std::string path = cfg.device;
    if( not path.starts_with( "\\\\.\\" ) ) path.insert( 0, "\\\\.\\" );

    serial_handle_t hdl = ::CreateFileA( path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    RGH_ASSERT_OR( hdl != SERIAL_INVALID_HANDLE ) {
        RGH_BRDG_LOGE( "serial: bad uplink to {}, error code [{}].", cfg.device, ::GetLastError() );
        return RGH_ERR_SYSCALL;
    }
    RGH_ON_SCOPE_EXIT_L( [ &hdl ] ( void ) -> void { ::CloseHandle( std::exchange( hdl, SERIAL_INVALID_HANDLE ) ); } );

    RGH_ASSERT_OR( ::FlushFileBuffers( hdl ) ) {
        RGH_BRDG_LOGW( "serial: bad buffer flush for {}, error code [{}].", cfg.device, ::GetLastError() );
    }

    COMMTIMEOUTS timeouts{
        .ReadIntervalTimeout         = cfg.rx_ib_timeout,
        .ReadTotalTimeoutMultiplier  = 0,
        .ReadTotalTimeoutConstant    = cfg.rx_fb_timeout,
        .WriteTotalTimeoutMultiplier = 0,
        .WriteTotalTimeoutConstant   = cfg.tx_timeout
    };
    RGH_ASSERT_OR( ::SetCommTimeouts( hdl, &timeouts ) ) {
        RGH_BRDG_LOGW( "serial: bad timeouts for {}, error code [{}].", cfg.device, ::GetLastError() );
    }

    DCB state; std::memset( &state, 0x0, sizeof( state ) ); state.DCBlength = sizeof( DCB );
    RGH_ASSERT_OR( ::GetCommState( hdl, &state ) ) {
        RGH_BRDG_LOGW( "serial: bad state populate for {}, some fields may be defaulted, error code [{}].", cfg.device, ::GetLastError() );
    }

    state.BaudRate     = static_cast<DWORD>( cfg.baud_rate );
    state.ByteSize     = cfg.byte_size;
    state.Parity       = cfg.parity;
    state.StopBits     = cfg.stopbit;
    state.fOutxCtsFlow = false;  
    state.fRtsControl  = RTS_CONTROL_DISABLE;
    state.fOutX        = false;  
    state.fInX         = false;   
    state.fBinary      = true;

    RGH_ASSERT_OR( ::SetCommState( hdl, &state ) ) {
        RGH_BRDG_LOGE( "serial: bad config for {}, error code [{}].", cfg.device, ::GetLastError() );
        return RGH_ERR_SYSCALL;
    }

    RGH_ON_SCOPE_EXIT_DROP;
    _handle = hdl;
    _config = std::move( cfg );
//# Post-uplink actions operate on the stored handle. Failures are not fatal to the uplink.
    if( _config.dtr_on_up != 0 ) RGH_ASSERT_RET_OR( this->dtr_toggle( _config.dtr_on_up ) ) {
        RGH_BRDG_LOGW( "serial: bad DTR toggle on uplink to {}.", _config.device );
    }
    if( _config.purge_on_up ) RGH_ASSERT_RET_OR( this->purge() ) {
        RGH_BRDG_LOGW( "serial: bad purge on uplink to {}.", _config.device );
    }

    RGH_BRDG_LOGI( "serial: uplinked {} @{}bauds.", _config.device, _config.baud_rate );
    return RGH_OK;
}

RGH_IMPL_FNC ret_t Simple_serial::downlink( void ) noexcept {
//# Assert that we were uplinked.    
    if( _handle == SERIAL_INVALID_HANDLE ) return RGH_OK;

    if( _config.purge_on_down ) RGH_ASSERT_RET_OR( this->purge() ) {
        RGH_BRDG_LOGW( "serial: bad purge on downlink from {}.", _config.device );
    }

    auto hdl = std::exchange( _handle, SERIAL_INVALID_HANDLE );
    auto cfg = std::exchange( _config, serial_config_t{} );

    RGH_ASSERT_OR( ::CloseHandle( hdl ) ) {
        RGH_BRDG_LOGE( "serial: bad downlink from {}, error code [{}].", cfg.device, ::GetLastError() );
        return RGH_ERR_SYSCALL;
    }

    RGH_BRDG_LOGI( "serial: downlinked {}.", cfg.device );
    return RGH_OK;
}

RGH_IMPL_FNC ret_t Simple_serial::read( const port_R_desc_t& desc_ ) {
    DWORD bc = 0;
    RGH_ASSERT_OR( ::ReadFile( _handle, desc_.dst_ptr, static_cast<DWORD>( desc_.dst_n ), &bc, nullptr ) ) {
        desc_.set_bc( 0 );
        return RGH_ERR_SYSCALL;
    }

    desc_.set_bc( static_cast<int>( bc ) );
    if( desc_.req_all and static_cast<int>( bc ) != desc_.dst_n ) return RGH_ERR_PARTIAL;

    return RGH_OK;
}

RGH_IMPL_FNC ret_t Simple_serial::write( const port_W_desc_t& desc_ ) {
    DWORD bc = 0;
    RGH_ASSERT_OR( ::WriteFile( _handle, desc_.src_ptr, static_cast<DWORD>( desc_.src_n ), &bc, nullptr ) ) {
        desc_.set_bc( 0 );
        return RGH_ERR_SYSCALL;
    }

    desc_.set_bc( static_cast<int>( bc ) );
    if( desc_.req_all and static_cast<int>( bc ) != desc_.src_n ) return RGH_ERR_BUSY;

    return RGH_OK;
}

RGH_IMPL_FNC std::expected<uint32_t, ret_t> Simple_serial::rx_q_sz( void ) const {
    COMSTAT stat; std::memset( &stat, 0x0, sizeof( stat ) );
    RGH_ASSERT_OR( ::ClearCommError( _handle, nullptr, &stat ) ) return std::unexpected{ RGH_ERR_SYSCALL };
    return static_cast<uint32_t>( stat.cbInQue );
}

RGH_IMPL_FNC ret_t Simple_serial::purge( void ) const {
    RGH_ASSERT_OR( ::PurgeComm( _handle, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR ) ) return RGH_ERR_SYSCALL;
    return RGH_OK;
}

//# Same line semantics as the Linux implementation: reset asserts DTR, set deasserts it.
RGH_IMPL_FNC ret_t Simple_serial::dtr_reset( void ) const {
    return ::EscapeCommFunction( _handle, SETDTR ) ? RGH_OK : RGH_ERR_SYSCALL;
}
RGH_IMPL_FNC ret_t Simple_serial::dtr_set( void ) const {
    return ::EscapeCommFunction( _handle, CLRDTR ) ? RGH_OK : RGH_ERR_SYSCALL;
}

#elifdef RGH_TARGET_OS_LINUX

//# _baud4termios is shared with rgh::io.

RGH_IMPL_FNC ret_t Simple_serial::uplink( 
    RGH_IN   serial_config_t   req_cfg_
) noexcept {
//# Validate the requested device path.
    RGH_ASSERT_OR( not req_cfg_.device.empty() ) { 
        RGH_BRDG_LOGE( "serial: empty device path." );
        return RGH_ERR_BADARG; 
    }
//# Assert that this port is not already uplinked. Relink or return based on the current config.
//# The configuration structure pointer must be valid if the handle is valid.
    if( _handle != SERIAL_INVALID_HANDLE ) {
        if( not _config.allow_up_ovrwr ) {
            RGH_BRDG_LOGE( "serial: uplink to {} would override existing link to {}.", req_cfg_.device, _config.device );
            return RGH_ERR_WOULD_OVRWR;
        }
        RGH_ASSERT_RET_OR( this->Simple_serial::downlink() ) {
            RGH_BRDG_LOGE( "serial: bad downlink when uplinking to {}.", req_cfg_.device );
            return ret_;
        }
    }

    serial_config_t cfg = std::move( req_cfg_ );

    serial_handle_t hdl = ::open( cfg.device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK );
    RGH_ASSERT_OR( hdl != SERIAL_INVALID_HANDLE ) {
        RGH_BRDG_LOGE( "serial: bad uplink to {}.", cfg.device );
        return RGH_ERR_SYSCALL;
    }
    RGH_ON_SCOPE_EXIT_L( [ &hdl ] ( void ) -> void { ::close( std::exchange( hdl, SERIAL_INVALID_HANDLE ) ); } );

    struct termios tty; std::memset( &tty, 0x0, sizeof( tty ) );
    RGH_ASSERT_OR( 0x0 == tcgetattr( hdl, &tty ) ) {
        RGH_BRDG_LOGE( "serial: bad attr populate for {}.", cfg.device );
        return RGH_ERR_SYSCALL;
    }

    cfmakeraw( &tty );
    tty.c_cflag &= ~CRTSCTS;

    const speed_t baud = _baud4termios( cfg.baud_rate );
    RGH_ASSERT_OR( 0x0 == cfsetospeed( &tty, baud ) and 0x0 == cfsetispeed( &tty, baud ) ) {
        RGH_BRDG_LOGE( "serial: bad baud for {}.", cfg.device );
        return RGH_ERR_SYSCALL;
    }

    tty.c_cflag &= ~CSIZE;
    switch( cfg.byte_size ) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        default: tty.c_cflag |= CS8;
    }

    switch( cfg.parity ) {
        case SERIAL_PARITY_NONE: tty.c_cflag &= ~PARENB; break;
        case SERIAL_PARITY_ODD: tty.c_cflag |= PARENB; tty.c_cflag |= PARODD; break;
        case SERIAL_PARITY_EVEN: tty.c_cflag |= PARENB; tty.c_cflag &= ~PARODD; break;
        case SERIAL_PARITY_MARK: tty.c_cflag |= PARENB; tty.c_cflag |= PARODD; tty.c_cflag |= CMSPAR; break;
        case SERIAL_PARITY_SPACE: tty.c_cflag |= PARENB; tty.c_cflag &= ~PARODD; tty.c_cflag |= CMSPAR; break;
        default: tty.c_cflag &= ~PARENB;
    }
   
    (void)( cfg.stopbit == SERIAL_STOPBIT_ONE ? tty.c_cflag &= ~CSTOPB : tty.c_cflag |= CSTOPB );

    tty.c_cflag |= ( CLOCAL | CREAD );
    tty.c_lflag &= ~( ICANON | ECHO | ECHOE | ISIG );
    tty.c_iflag &= ~( IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL );
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = (cc_t)( cfg.rx_fb_timeout / 100 ); if( tty.c_cc[VTIME] == 0 ) tty.c_cc[VTIME] = 1;

    RGH_ASSERT_OR( 0x0 == tcsetattr( hdl, TCSANOW, &tty ) ) {
        RGH_BRDG_LOGE( "serial: bad config for {}.", cfg.device );
        return RGH_ERR_SYSCALL;
    }

    RGH_ON_SCOPE_EXIT_DROP;
    _handle = hdl;
    _config = std::move( cfg );
//# Post-uplink actions operate on the stored handle. Failures are not fatal to the uplink.
    if( _config.dtr_on_up != 0 ) RGH_ASSERT_RET_OR( this->dtr_toggle( _config.dtr_on_up ) ) {
        RGH_BRDG_LOGW( "serial: bad DTR toggle on uplink to {}.", _config.device );
    }
    if( _config.purge_on_up ) RGH_ASSERT_RET_OR( this->purge() ) {
        RGH_BRDG_LOGW( "serial: bad purge on uplink to {}.", _config.device );
    }

    RGH_BRDG_LOGI( "serial: uplinked {}.", _config.device );
    return RGH_OK;
}

RGH_IMPL_FNC ret_t Simple_serial::downlink( void ) noexcept {
//# Assert that we were uplinked.    
    if( _handle == SERIAL_INVALID_HANDLE ) return RGH_OK;

    if( _config.purge_on_down ) RGH_ASSERT_RET_OR( this->purge() ) {
        RGH_BRDG_LOGW( "serial: bad purge on downlink from {}.", _config.device );
    }

    auto hdl = std::exchange( _handle, SERIAL_INVALID_HANDLE );
    auto cfg = std::exchange( _config, serial_config_t{} );

    RGH_ASSERT_OR( 0x0 == ::close( hdl ) ) {
        RGH_BRDG_LOGE( "serial: bad downlink from {}.", cfg.device );
        return RGH_ERR_SYSCALL;
    }

    RGH_BRDG_LOGI( "serial: downlinked {}.", cfg.device );
    return RGH_OK;
}

RGH_IMPL_FNC ret_t Simple_serial::read( const port_R_desc_t& desc_ ) {
    ssize_t bc = ::read( _handle, desc_.dst_ptr, desc_.dst_n );

    RGH_ASSERT_OR( bc >= 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK ) { bc = 0; goto l_ok; }
        return RGH_ERR_SYSCALL;
    }

l_ok:
    desc_.set_bc( bc );
    if( desc_.req_all and bc != desc_.dst_n ) return RGH_ERR_PARTIAL;
    
    return RGH_OK;
}

RGH_IMPL_FNC ret_t Simple_serial::write( const port_W_desc_t& desc_ ) {
    ssize_t bc = ::write( _handle, desc_.src_ptr, desc_.src_n );

    RGH_ASSERT_OR( bc >= 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK ) { bc = 0; goto l_ok; }
        return RGH_ERR_SYSCALL;
    }

l_ok:
    desc_.set_bc( bc );
    if( desc_.req_all and bc != desc_.src_n ) return RGH_ERR_BUSY;
    
    return RGH_OK;
}

RGH_IMPL_FNC std::expected<uint32_t, ret_t> Simple_serial::rx_q_sz( void ) const {
    int bca = 0;
    RGH_ASSERT_OR( ::ioctl( _handle, FIONREAD, &bca ) >= 0x0 ) return std::unexpected{ RGH_ERR_SYSCALL };
    RGH_ASSERT_OR( bca >= 0 ) return std::unexpected{ RGH_ERR_CORRUPTED };
    return static_cast<uint32_t>( bca );
}

RGH_IMPL_FNC ret_t Simple_serial::purge( void ) const {
    RGH_ASSERT_OR( ::tcflush( _handle, TCIOFLUSH ) == 0 ) return RGH_ERR_SYSCALL;
    return RGH_OK;
}

RGH_IMPL_FNC ret_t Simple_serial::dtr_reset( void ) const {
    int status = 0x0;
    RGH_ASSERT_OR( 0x0 == ioctl( _handle, TIOCMGET, &status ) ) return RGH_ERR_SYSCALL;
    status |= TIOCM_DTR;
    return ioctl( _handle, TIOCMSET, &status ) == 0x0 ? RGH_OK : RGH_ERR_SYSCALL;
}
RGH_IMPL_FNC ret_t Simple_serial::dtr_set( void ) const {
    int status = 0x0;
    RGH_ASSERT_OR( 0x0 == ioctl( _handle, TIOCMGET, &status ) ) return RGH_ERR_SYSCALL;
    status &= ~TIOCM_DTR;
    return ioctl( _handle, TIOCMSET, &status ) == 0x0 ? RGH_OK : RGH_ERR_SYSCALL;
}
#endif

#if !defined( RGH_IMPL_SEL_IO_V2_SERIAL_DTR_TOGGLE )
#define RGH_IMPL_SEL_IO_V2_SERIAL_DTR_TOGGLE
RGH_IMPL_FNC ret_t Simple_serial::dtr_toggle( 
    RGH_IN   const uint32_t   usec_ 
) const {
    RGH_ASSERT_RET_OR_RET( this->dtr_reset() );
    std::this_thread::sleep_for( std::chrono::microseconds( usec_ ) );
    return this->dtr_set();
}
#endif//# RGH_IMPL_SEL_IO_V2_SERIAL_DTR_TOGGLE

#pragma endregion v2::Simple_serial

#pragma region v2::Fasttrack_serial

#ifdef RGH_TARGET_OS_WINDOWS

#elifdef RGH_TARGET_OS_LINUX

#ifndef RGH_IMPL_SEL_IO_V2_FASTTRACK_SERIAL__POLL_LOOP
#define RGH_IMPL_SEL_IO_V2_FASTTRACK_SERIAL__POLL_LOOP
RGH_IMPL_FNC void Fasttrack_serial::_poll_loop( 
    RGH_IN_OUT   std::stop_token   stop_tok_ 
) noexcept {
//# Local copy, only used for logging.
    const serial_config_t cfg = Simple_serial::_config;

    pollfd pfd = {
        .fd      = _handle,
        .events  = POLLIN,
        .revents = 0
    };

    while( not stop_tok_.stop_requested() ) {
        int plr = ::poll( &pfd, 1, 500 );

        RGH_ASSERT_AND( plr > 0 ) {
            RGH_ASSERT_OR( (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) == 0x0 ) {
                RGH_BRDG_LOGE( "ft_serial: sudden disconnect of {}.", cfg.device );
                break;
            }

            RGH_ASSERT_OR( pfd.revents & POLLIN ) continue;

        //# Non-blocking read, returns whatever is available up to the buffer size.
            byte_t buffer[ 0x400 ];
            int    bc = 0;
            RGH_ASSERT_STATUS_OR( this->read( {
                .dst_ptr    = buffer,
                .dst_n      = static_cast<int>( sizeof( buffer ) ),
                .byte_count = &bc
            } ) ) continue;

            RGH_ASSERT_AND( bc > 0 ) this->_bytes_cb( std::span<const byte_t>{ buffer, static_cast<size_t>( bc ) } );

        } else RGH_ASSERT_AND( plr == 0 ) {
            /*# Timeout on the poll. */
        } else {
            if( errno == EINTR ) continue;
            RGH_BRDG_LOGE( "ft_serial: bad poll on {}: {}.", cfg.device, std::strerror( errno ) );
            break;
        }
    }
    RGH_BRDG_LOGW( "ft_serial: poll loop exited on {}.", cfg.device );
}
#endif//# RGH_IMPL_SEL_IO_V2_FASTTRACK_SERIAL__POLL_LOOP

#endif//# RGH_TARGET_OS

RGH_IMPL_FNC void Fasttrack_serial::_poll_stop( void ) noexcept {
    if( not _poll_th.joinable() ) return;
    _poll_th.request_stop();
    _poll_th.join();
    _poll_th = {};
}

RGH_IMPL_FNC ret_t Fasttrack_serial::uplink( 
    RGH_IN   serial_config_t   req_cfg_
) noexcept {
    RGH_ASSERT_OR( _bytes_cb ) {
        RGH_BRDG_LOGE( "ft_serial: no bytes callback for uplink to {}.", req_cfg_.device );
        return RGH_ERR_BADARG;
    }
//# The poll thread must not outlive the handle it polls. Restart it on whatever link remains afterwards.
    this->_poll_stop();
    const ret_t ret = this->Simple_serial::uplink( std::move( req_cfg_ ) );
    if( not this->is_uplinked() ) return ret;

    _poll_th = std::jthread{ [ this ] ( std::stop_token stop_tok_ ) -> void { this->_poll_loop( std::move( stop_tok_ ) ); } };
    RGH_BRDG_LOGI( "ft_serial: poll loop entered on {}.", _config.device );
    return ret;
}

RGH_IMPL_FNC ret_t Fasttrack_serial::uplink( 
    RGH_IN   serial_config_t   req_cfg_,
    RGH_IN   bytes_cb_t        bytes_cb_
) noexcept {
    RGH_ASSERT_OR( bytes_cb_ ) {
        RGH_BRDG_LOGE( "ft_serial: no bytes callback for uplink to {}.", req_cfg_.device );
        return RGH_ERR_BADARG;
    }
    this->_poll_stop();
    _bytes_cb = std::move( bytes_cb_ );
    return this->Fasttrack_serial::uplink( std::move( req_cfg_ ) );
}

RGH_IMPL_FNC ret_t Fasttrack_serial::downlink( void ) noexcept {
    this->_poll_stop();
    return this->Simple_serial::downlink();
}

#ifndef RGH_IMPL_SEL_IO_V2_FASTTRACK_SERIAL__POLL_LOOP
#define RGH_IMPL_SEL_IO_V2_FASTTRACK_SERIAL__POLL_LOOP
RGH_IMPL_FNC void Fasttrack_serial::_poll_loop(
    RGH_IN_OUT   std::stop_token   stop_tok_
) noexcept {
//# Local copy, only used for logging.
    const serial_config_t cfg = Simple_serial::_config;

    while( not stop_tok_.stop_requested() ) {
        for(;;) {
            auto avail = this->rx_q_sz();
            RGH_ASSERT_OR( avail ) {
                RGH_BRDG_LOGE( "ft_serial: sudden disconnect of {}.", cfg.device );
                goto l_exit;
            }
            if( *avail == 0 ) break;

            byte_t buffer[ 0x400 ];
            int    bc = static_cast<int>( std::min<uint32_t>( *avail, sizeof( buffer ) ) );
            RGH_ASSERT_STATUS_OR( this->read( {
                .dst_ptr    = buffer,
                .dst_n      = bc,
                .byte_count = &bc
            } ) ) break;

            RGH_ASSERT_OR( bc > 0 ) break;
            this->_bytes_cb( std::span<const byte_t>{ buffer, static_cast<size_t>( bc ) } );
        }
        std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
    }

l_exit:
    RGH_BRDG_LOGW( "ft_serial: poll loop exited on {}.", cfg.device );
}
#endif//# RGH_IMPL_SEL_IO_V2_FASTTRACK_SERIAL__POLL_LOOP

#pragma endregion v2::Fasttrack_serial

}
#else
    #warning "[RGH] This source file is not compilable."
#endif//#_RGH_SOURCE_IS_COMPILABLE
