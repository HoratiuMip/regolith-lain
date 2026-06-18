#pragma once
/**
 * @file: ucp/esp32-5x/sns-drv/bmp280.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/ucp/esp32-5x/IO_i2c.hpp>
#include <rgh/ucp/sns-drv/bmp280.hpp>

namespace rgh::esp32::snsd { using namespace rgh::esp32::io;

class BMP280 : public rgh::snsd::BMP280 {
_RGH_PROTECTED:
    I2C_m2s   _i2c   = {};   

public:
    status_t bind( i2c_master_bus_handle_t i2c_bus_, i2c_device_config_t dev_cfg_, int to_ = I2C_m2s::DEFAULT_TIMEOUT ) {
        RGH_ASSERT_STATUS_OR_RET( _i2c.bind( i2c_bus_, dev_cfg_, to_ ) );
        RGH_ASSERT_STATUS_OR_RET( this->bind_i2c( &_i2c ) );
        return RGH_OK;
    }

};

};