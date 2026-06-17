#pragma once
/**
 * @file: ucp/esp32-5x/uP_temp.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <driver/temperature_sensor.h>
#include <rgh/ucp/core.hpp>

namespace rgh::esp32 {

#define RGH_ESP32_UP_TEMP_SNS_RANGE_50_125 .range_min=50,.range_max=125
#define RGH_ESP32_UP_TEMP_SNS_RANGE_20_100 .range_min=20,.range_max=100
#define RGH_ESP32_UP_TEMP_SNS_RANGE_n10_80 .range_min=-10,.range_max=80
#define RGH_ESP32_UP_TEMP_SNS_RANGE_n30_50 .range_min=-30,.range_max=50
#define RGH_ESP32_UP_TEMP_SNS_RANGE_n40_20 .range_min=-40,.range_max=20

static temperature_sensor_handle_t   _uP_temp_sensor_handle   = nullptr;

status_t uP_temp_sensor_init( 
    RGH_IN   const temperature_sensor_config_t&   cfg_
) noexcept {
    RGH_ASSERT_OR( ESP_OK == temperature_sensor_install( &cfg_, &_uP_temp_sensor_handle ) ) return RGH_ERR_PLATFORMCALL;
    temperature_sensor_enable( _uP_temp_sensor_handle );
    return RGH_OK;
}

void uP_temp_sensor_deinit(
    void
) noexcept {
    RGH_ASSERT_OR( _uP_temp_sensor_handle ) return;
    temperature_sensor_uninstall( _uP_temp_sensor_handle );
    temperature_sensor_disable( _uP_temp_sensor_handle );
    _uP_temp_sensor_handle = nullptr;
}

std::optional< float > uP_temp_read( 
    void 
) noexcept {
    float out = 0.0f; 
    RGH_ASSERT_OR( ESP_OK == temperature_sensor_get_celsius( _uP_temp_sensor_handle, &out ) ) return {}; 
    return out;
}

};