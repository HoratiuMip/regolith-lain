#pragma once
/**
 * @file: ucp/esp32-5x/button.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#if __has_include( "iot_button.h" ) && __has_include( "button_gpio.h" )
    #include "iot_button.h"
    #include "button_gpio.h"
#else
    #error "Ensure the IoT button dependency is pulled via idf_component.yml: dependencies: espressif/button: \"^4.1.6\""
#endif

namespace rgh::esp32 {

struct button_t {
public:
    button_t( const button_gpio_config_t& gpio_cfg_, const button_config_t& btn_cfg_ ) {
        iot_button_new_gpio_device( &btn_cfg_, &gpio_cfg_, &_handle );
        RGH_ASSERT_OR( _handle ) return;
    }

    ~button_t( void ) {
        if( _handle ) iot_button_delete( std::exchange( _handle, nullptr ) );
    }

_RGH_PROTECTED:
    button_handle_t   _handle   = nullptr;

public:
    RGH_inline button_handle_t handle( void ) const { return _handle; } 
    RGH_inline operator button_handle_t ( void ) const { return _handle; }

    RGH_inline operator bool( void ) const { return _handle != nullptr; }

public:
    RGH_inline auto event( void ) { return iot_button_get_event( _handle ); }

};

};