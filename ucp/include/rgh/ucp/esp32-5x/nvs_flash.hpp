#pragma once
/**
 * @file: ucp/esp32-5x/flash.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/ucp/core.hpp>

#include <nvs_flash.h>

#if __has_include( "Preferences.h" )
    #define RGH_ESP32_EEPROM_AVAILABLE
    #include "Preferences.h"
#endif

namespace rgh::esp32 {

status_t init_nvs_flash( void ) {
    esp_err_t info = nvs_flash_init();
    if( ESP_ERR_NVS_NO_FREE_PAGES == info || ESP_ERR_NVS_NEW_VERSION_FOUND == info ) {
        RGH_ASSERT_OR( ESP_OK == nvs_flash_erase() ) return RGH_ERR_PLATFORMCALL;
        info = nvs_flash_init();
    }
    RGH_ASSERT_OR( ESP_OK == info ) return RGH_ERR_PLATFORMCALL;
    return RGH_OK;
}

RGH_inline status_t erase_nvs_flash( void ) {
    return ESP_OK == nvs_flash_erase() ? RGH_OK : RGH_ERR_PLATFORMCALL;
}

#ifdef RGH_ESP32_EEPROM_AVAILABLE

#define   RGH_ESP32_EEPROM_DEFAULT_SECTOR   "defs"

class _EEPROM : public Preferences {
_RGH_PROTECTED:
    std::string   _sec   = RGH_ESP32_EEPROM_DEFAULT_SECTOR;
    std::mutex    _mtx   = {};

public:
    RGH_inline void swap_sector( const auto& sec_ ) { _sec = sec_; }

public:
    RGH_inline void lock( void ) { _mtx.lock(); this->begin( _sec.c_str(), true ); }
    RGH_inline void unlock( void ) { this->end(); _mtx.unlock(); }
    RGH_inline bool try_lock( void ) { 
        RGH_ASSERT_OR( _mtx.try_lock() ) return false;
        this->begin( _sec.c_str(), true ); return true;
    }

    RGH_inline void lock_shared( void ) { _mtx.lock(); this->begin( _sec.c_str(), false ); }
    RGH_inline void unlock_shared( void ) { this->end(); _mtx.unlock(); }
    RGH_inline bool try_lock_shared( void ) { 
        RGH_ASSERT_OR( _mtx.try_lock() ) return false;
        this->begin( _sec.c_str(), false ); return true;
    }

public:
    template< size_t MAX_LEN_ = 15 > std::string getString( const char* key_, std::string def_ = {} ) {
        char buf[ MAX_LEN_ + 1 ]; 

        size_t len = this->Preferences::getString( key_, buf, MAX_LEN_ );
        RGH_ASSERT_OR( 0x0 != len ) return def_;

        return { buf, len };
    }

};
inline _EEPROM EEPROM;

#endif

};