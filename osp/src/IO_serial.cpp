/**
 * @file: osp/IO_serial.cpp
 * @brief: Implementation file.
 * @details: -
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/IO_serial.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

namespace rgh::io {

#ifdef RGH_TARGET_OS_WINDOWS
status_t Serial::open( const char* device_, const serial_config_t& config_ ) {
    if( _port != SERIAL_INVALID_HANDLE ) this->close();

    _port = CreateFileA( device_, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    RGH_ON_SCOPE_EXIT_L( [ this ] ( void ) -> void {
        if( _port != SERIAL_INVALID_HANDLE ) {
            CloseHandle( std::exchange( _port, SERIAL_INVALID_HANDLE ) );
        }
    } );

    RGH_ASSERT_OR( _port != SERIAL_INVALID_HANDLE ) {
        RGH_LOGE_IO_EX( RGH_ERR_SYSCALL, "Could not open serial port \"{}\".", device_ );
        return RGH_ERR_SYSCALL;
    }

    RGH_ASSERT_OR( FlushFileBuffers( _port ) ) {
        RGH_LOGW_IO( "Could not flush serial port buffers for opened serial port \"{}\", error code [{}].", device_, GetLastError() );
    }

    COMMTIMEOUTS timeouts{
        ReadIntervalTimeout         : config_.rx_ib_timeout,
        ReadTotalTimeoutMultiplier  : 0,
        ReadTotalTimeoutConstant    : config_.rx_fb_timeout,
        WriteTotalTimeoutMultiplier : 0,
        WriteTotalTimeoutConstant   : config_.tx_timeout
    };

    RGH_ASSERT_OR( SetCommTimeouts( _port, &timeouts ) ) {
        RGH_LOGW_IO( "Could not set the configured timeouts for serial port \"{}\", error code [{}].", device_, GetLastError() );
    }

    DCB state{ 0 }; state.DCBlength = sizeof( DCB );

    RGH_ASSERT_OR( GetCommState( _port, &state ) ) {
        RGH_LOGW_IO( "Could not read the default state of the serial port \"{}\". Some configurations might be defaulted or contain garbage values. Error code [{}].", device_, GetLastError() );
    }

    state.BaudRate     = ( uint32_t )config_.baud_rate;
    state.ByteSize     = config_.byte_size;
    state.Parity       = config_.parity;
    state.StopBits     = config_.stopbit;
    state.fOutxCtsFlow = false;  
    state.fRtsControl  = RTS_CONTROL_DISABLE;
    state.fOutX        = false;  
    state.fInX         = false;   

    RGH_ASSERT_OR( SetCommState( _port, &state ) ) {
        RGH_LOGE_IO( "Could not configure the serial port \"{}\", error code [{}].", device_, GetLastError() );
        return -0x1;
    }

    if( config_.purge_on_open ) this->purge();

    RGH_ON_SCOPE_EXIT_DROP;
    _device = device_;
    _config = config_;

    RGH_LOGI_IO( "Opened and configured serial port \"{}\" successfully @{}bauds.", _device, _config.baud_rate );
    return 0x0;
}

status_t Serial::close( void ) {
    if( _port == SERIAL_INVALID_HANDLE ) return 0x0;
    
    if( _config.purge_on_close ) this->purge();
    CloseHandle( std::exchange( _port, INVALID_HANDLE_VALUE ) );
    RGH_LOGI_IO( "Closed serial port \"{}\".", _device );
    _device = "";
    _config = serial_config_t{};
    return 0x0;
}

status_t Serial::read( const port_R_desc_t& desc_ ) {
    uint32_t byte_count = 0;
    ReadFile( _port, desc_.dst_ptr, desc_.dst_n, ( LPDWORD )&byte_count, nullptr );
    
    if( desc_.byte_count ) *desc_.byte_count = byte_count;

    if( desc_.req_all && byte_count != desc_.dst_n ) {
        return RGH_ERR_FLOW;
    }

    return RGH_OK;
}

status_t Serial::write( const port_W_desc_t& desc_ ) {
    uint32_t byte_count = 0;
    WriteFile( _port, desc_.src_ptr, desc_.src_n, ( LPDWORD )&byte_count, nullptr );

    if( desc_.byte_count ) *desc_.byte_count = byte_count;

    if( desc_.req_all && byte_count != desc_.src_n ) {
        return -0x1;
    }

    return 0x0;
}

int Serial::rx_available( void ) const {
    COMSTAT stat; memset( &stat, 0x0, sizeof( COMSTAT ) );
    RGH_ASSERT_OR( ClearCommError( _port, nullptr, &stat ) ) return -0x1;
    return stat.cbInQue;
}

status_t Serial::purge( void ) const {
    RGH_ASSERT_OR( PurgeComm( _port, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR ) ) {
        RGH_LOGW_IO( "Could not purge serial port \"{}\", error code [{}].", _device, GetLastError() );
        return -0x1;
    }
    return 0x0;
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

status_t Serial::open( 
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

    if( _config.purge_on_open ) {
        RGH_ASSERT_STATUS_OR( this->purge() ) {
            RGH_BRDG_LOGW( "serial: bad purge when opening {}.", _device );
        }
    }

    RGH_ON_SCOPE_EXIT_DROP;
    RGH_BRDG_LOGI( "serial: opened {}.", _device );
    return RGH_OK;
}

status_t Serial::close( void ) {
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

status_t Serial::read( const port_R_desc_t& desc_ ) {
    ssize_t bc = ::read( _port, desc_.dst_ptr, desc_.dst_n );

    RGH_ASSERT_OR( bc >= 0 ) {
        if( errno == EAGAIN || errno == EWOULDBLOCK ) { bc = 0; goto l_ok; }
        return RGH_ERR_SYSCALL;
    }

l_ok:
    desc_.set_bc( bc );
    if( desc_.req_all and bc != desc_.dst_n ) return RGH_ERR_DEPLETED;
    
    return RGH_OK;
}

int Serial::write( const port_W_desc_t& desc_ ) {
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

int Serial::rx_available( void ) const {
    int bca = 0;
    RGH_ASSERT_OR( ioctl( _port, FIONREAD, &bca ) >= 0x0 ) return 0;
    return bca;
}

status_t Serial::purge( void ) const {
    RGH_ASSERT_OR( tcflush( _port, TCIOFLUSH ) == 0 ) return RGH_ERR_SYSCALL;
    return RGH_OK;
}

#endif

}
