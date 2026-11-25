#pragma once /*
# FILE: osp/IO_serial.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Serial.
*/

#include <rgh/osp/core.hpp>

#include <rgh/brp/IO_port.hpp>
#include <rgh/brp/IO_utils.hpp>

#include <expected>
#include <functional>
#include <span>
#include <string>
#include <thread>

#ifdef RGH_TARGET_OS_WINDOWS
    typedef   ::HANDLE   serial_handle_t;

    inline const serial_handle_t   SERIAL_INVALID_HANDLE    = INVALID_HANDLE_VALUE;

    inline constexpr uint8_t       SERIAL_PARITY_NONE       = NOPARITY; 
    inline constexpr uint8_t       SERIAL_PARITY_ODD        = ODDPARITY; 
    inline constexpr uint8_t       SERIAL_PARITY_EVEN       = EVENPARITY; 
    inline constexpr uint8_t       SERIAL_PARITY_MARK       = MARKPARITY; 
    inline constexpr uint8_t       SERIAL_PARITY_SPACE      = SPACEPARITY; 

    inline constexpr uint8_t       SERIAL_STOPBIT_ONE       = ONESTOPBIT;
    inline constexpr uint8_t       SERIAL_STOPBIT_ONE_HALF  = ONE5STOPBITS;
    inline constexpr uint8_t       SERIAL_STOPBIT_TWO       = TWOSTOPBITS;

#elifdef RGH_TARGET_OS_LINUX
    typedef   int   serial_handle_t;

    inline const serial_handle_t   SERIAL_INVALID_HANDLE    = -1;

    inline constexpr uint8_t       SERIAL_PARITY_NONE       = 0x0; 
    inline constexpr uint8_t       SERIAL_PARITY_ODD        = 0x1; 
    inline constexpr uint8_t       SERIAL_PARITY_EVEN       = 0x2; 
    inline constexpr uint8_t       SERIAL_PARITY_MARK       = 0x3; 
    inline constexpr uint8_t       SERIAL_PARITY_SPACE      = 0x4; 

    inline constexpr uint8_t       SERIAL_STOPBIT_ONE       = 0;
    inline constexpr uint8_t       SERIAL_STOPBIT_TWO       = 0;
#endif 

namespace rgh::io { 

struct serial_config_t {
    uint32_t   baud_rate       = 115200;
    uint32_t   rx_fb_timeout   = 1000;
    uint32_t   rx_ib_timeout   = 10;
    uint32_t   tx_timeout      = 1000;
    uint8_t    byte_size       = 8;
    uint8_t    parity          = SERIAL_PARITY_NONE;
    uint8_t    stopbit         = SERIAL_STOPBIT_ONE;
    uint32_t   dtr_on_open     = 0;
    bool       purge_on_open   = true;
    bool       purge_on_close  = false;
};

class Serial : public Port {
public:
    Serial() = default;

    Serial( const char* device_, const serial_config_t& config_ ) {
        this->open( device_, config_ );
    }

    ~Serial() {
        this->close();
    }

_RGH_PROTECTED:
    serial_handle_t   _port       = SERIAL_INVALID_HANDLE;
    std::string       _device     = "";
    serial_config_t   _config     = {};

public:
    /**
     * @brief: Returns the handle type native to the underlying operating system.
     */
    RGH_inline serial_handle_t native_handle( void ) const { return _port; }

    RGH_inline std::string_view device( void ) const { return _device; }

public:
    RGH_inline bool is_connected( void ) const { return _port != SERIAL_INVALID_HANDLE; }

public:
    status_t open( const char* device_, const serial_config_t& config_ );

    status_t close( void );

public:
    status_t read( const port_R_desc_t& desc_ ) override;
  
    status_t write( const port_W_desc_t& desc_ ) override;

public:
    /**
     * @brief: Returns the number of bytes waiting to be read from the reception buffer.
     */
    int rx_q_sz( void ) const;

    /**
     * @brief: Clear the transmission and reception buffers.
     */
    status_t purge( void ) const;

/// Bring the DTR line to LOW.
    ret_t dtr_reset( void ) const;
/// Bring the DTR line to HIGH.
    ret_t dtr_set( void ) const;
/// Toggle the DTR line.
/// @param usec_: Time in microseconds to sleep between the DTR line change.
    ret_t dtr_toggle( const uint32_t usec_ ) {
        RGH_ASSERT_RET_OR_RET( this->dtr_reset() );
        std::this_thread::sleep_for( std::chrono::microseconds( usec_ ) );
        return this->dtr_set();
    }

};

class Fasttrack_serial : public Serial {
public:
    typedef   std::function< void( const byte_t*, int ) >   bytes_cb_t;
_RGH_PROTECTED:
    bytes_cb_t     _bytes_cb   = nullptr;
    std::jthread   _poll_th    = {};

_RGH_PROTECTED:
    void _poll_loop( 
        RGH_IN_OUT   std::stop_token stop_tok_ 
    ) noexcept;

public:
    status_t open(
        RGH_IN   const char*              device_, 
        RGH_IN   const serial_config_t&   config_,
        RGH_IN   bytes_cb_t               bytes_cb_
    ) noexcept;

};

}

namespace rgh::io::v2 {

/// Serial port configuration strucutre.
/// ---
struct serial_config_t {
//# The device path.
    std::string   device   = "";
//# Communication speed.
    uint32_t      baud_rate       = 115200;
//# First byte reception timeout. Effectively, after how many milliseconds a call to read will return if no data is available.
    uint32_t      rx_fb_timeout   = 1000;
//# Inter byte reception timeout. 
    uint32_t      rx_ib_timeout   = 10;
//# Transmission timeout.
    uint32_t      tx_timeout      = 1000;
//# The size of what is considered a byte.
    uint8_t       byte_size       = 8;
//# Parity bit configuration.
    uint8_t       parity          = SERIAL_PARITY_NONE;
//# Stop bits configuration.
    uint8_t       stopbit         = SERIAL_STOPBIT_ONE;
//# When this is not 0, it is the time in microseconds of the DTR line toggle to be performed on uplink.
    uint32_t      dtr_on_up       = 0;
//# Clear all buffers on uplink.
    bool          purge_on_up     = true;
//# Clear all buffers on downlink.
    bool          purge_on_down   = false;
//# Whether to allow through another uplink while already uplinked.
    bool          allow_up_ovrwr  = false;
};

/// Serial port interface.
/// ---
class Interface_serial : public Port {
public:
    virtual ~Interface_serial( void ) = default;

public:
/// Returns the underlying platform specific handle type.
/// ---
    virtual serial_handle_t native( void ) const = 0;

/// Returns a reference to the current configuration.
/// ---
    virtual HVec<const serial_config_t> config( void ) const = 0;

/// Returns whether the object has an uplink.
/// ---
    virtual bool is_uplinked( void ) const = 0;

/// Returns the number of bytes waiting in the reception buffer.
/// ---
    virtual std::expected<uint32_t, ret_t> rx_q_sz( void ) const = 0;

public:
/// Open the connection to the device with the given configuration.
/// ---
    virtual ret_t uplink( 
        RGH_IN   serial_config_t 
    ) noexcept = 0;

/// Close the connection to the device.
/// ---
    virtual ret_t downlink( void ) noexcept = 0;

/// Clear both transmission and reception buffers.
/// ---
    virtual ret_t purge( void ) const = 0;

/// Bring the DTR line to LOW.
/// ---
    virtual ret_t dtr_reset( void ) const = 0;
/// Bring the DTR line to HIGH.
/// ---
    virtual ret_t dtr_set( void ) const = 0;
/// Toggle the DTR line.
/// @param usec_ How long to keep the DTR line LOW, in microseconds.
/// ---
    virtual ret_t dtr_toggle( 
        RGH_IN   const uint32_t 
    ) const = 0;
};

/// Serial port object.
/// ---
class Simple_serial : public Interface_serial {
public:
    Simple_serial() = default;

    Simple_serial( 
        RGH_IN   serial_config_t   config_ 
    ) {
        this->Simple_serial::uplink( std::move( config_ ) );
    }

    ~Simple_serial( void ) override {
        this->Simple_serial::downlink();
    }

RGH_protected:
    serial_handle_t   _handle   = SERIAL_INVALID_HANDLE;
    serial_config_t   _config   = {};

public:
    RGH_inline serial_handle_t native( void ) const override { return _handle; }

    RGH_inline HVec<const serial_config_t> config( void ) const override { return _config; }

    RGH_inline bool is_uplinked( void ) const override { return _handle != SERIAL_INVALID_HANDLE; }

    std::expected<uint32_t, ret_t> rx_q_sz( void ) const override;

public:
    ret_t uplink( 
        RGH_IN   serial_config_t 
    ) noexcept override;

    ret_t downlink( void ) noexcept override;

    ret_t purge( void ) const override;

    ret_t dtr_reset( void ) const override;
    ret_t dtr_set( void ) const override;
    ret_t dtr_toggle( 
        RGH_IN   const uint32_t   usec_ 
    ) const override;

public:
    ret_t read( const port_R_desc_t& ) override;
  
    ret_t write( const port_W_desc_t& ) override;
};

/// Serial port which polls for incoming bytes on a separate thread, delivering them through a callback.
/// ---
class Fasttrack_serial : public Simple_serial {
public:
    typedef   std::function< void( std::span<const byte_t> ) >   bytes_cb_t;

public:
    Fasttrack_serial() = default;

    ~Fasttrack_serial( void ) override {
        this->Fasttrack_serial::downlink();
    }

RGH_protected:
    bytes_cb_t     _bytes_cb   = nullptr;
    std::jthread   _poll_th    = {};

RGH_protected:
    void _poll_loop( 
        RGH_IN_OUT   std::stop_token 
    ) noexcept;

    void _poll_stop( void ) noexcept;

public:
/// Open the connection to the device with the given configuration, keeping the current callback.
/// ---
    ret_t uplink(
        RGH_IN   serial_config_t
    ) noexcept override;

/// Open the connection to the device with the given configuration and callback.
/// ---
    ret_t uplink(
        RGH_IN   serial_config_t,
        RGH_IN   bytes_cb_t
    ) noexcept;

/// Stop the poll thread and close the connection to the device.
/// ---
    ret_t downlink( void ) noexcept override;

};

}
