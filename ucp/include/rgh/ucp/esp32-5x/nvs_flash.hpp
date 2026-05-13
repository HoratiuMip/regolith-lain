#pragma once
/**
 * @file: ucp/esp32-5x/flash.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <nvs_flash.h>

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

};