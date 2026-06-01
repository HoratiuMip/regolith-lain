#pragma once
/**
 * @file: gep/wifi_daemon.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <WiFi.h>

#include <rgh/brp/IO_string_utils.hpp>
#include <rgh/ucp/core.hpp>
#include <rgh/gep/daemon.hpp>

namespace rgh::ino {

class WiFi_daemon : public Daemon {
public:
    virtual std::string_view daemon_name( void ) const override { return "WiFi"; }
    virtual std::string daemon_report( [[maybe_unused]]void* ) const override {
        const auto crt_rssi = this->rssi();

        return std::format( 
            "=-. WiFi .-=\n"
            "\t- ssid: {} , connected: {} , rssi: {}dBm {}\n"
            "\t- ip: {} , channel: {}\n"
            , 
            this->ssid().c_str(), this->connected(), crt_rssi, ( const char* )io::wifi_rssi_str_t{ crt_rssi },
            WiFi.localIP().toString().c_str(), WiFi.channel()
        );
    }

public:
    RGH_inline bool connected( void ) const { return WiFi.status() == WL_CONNECTED; }

    RGH_inline int32_t rssi( void ) const { return WiFi.RSSI(); }
    RGH_inline String ssid( void ) const { return WiFi.SSID(); }

public:
    struct Dridge {
        virtual const char* get_ssid( void )            const = 0;
        virtual const char* get_pwrd( void )            const = 0; 
        virtual uint16_t    get_start_verify_ms( void ) const = 0;
        virtual uint8_t     get_start_attempts( void )  const = 0; 
    };

_RGH_PROTECTED:
    virtual status_t _daemon_start( void* arg_ ) override {
        ESP_LOGI( Tag, "WiFi: starting..." );

        auto DRG = ( const Dridge* )arg_;

        auto ssid = DRG->get_ssid();
        auto pwrd = DRG->get_pwrd();

        RGH_ASSERT_OR( ssid and pwrd ) { ESP_LOGE( Tag, "WiFi: bad creds." ); return RGH_ERR_NOT_FOUND; }

        WiFi.mode( WIFI_STA );
        WiFi.setAutoReconnect( true );
        WiFi.begin( ssid, pwrd );

        uint16_t attempt = 0;
        do {
            vTaskDelay( pdMS_TO_TICKS( DRG->get_start_verify_ms() ) );
            RGH_ASSERT_OR( ++attempt < DRG->get_start_attempts() ) {
                ESP_LOGE( Tag, "WiFi: could not connect: %s", ssid );
                return RGH_ERR_PLATFORMCALL;
            }
        } while( not this->connected() );

        ESP_LOGI( Tag, "WiFi: connected: %s.", ssid );
        return RGH_OK;
    }

    virtual status_t _daemon_stop( [[maybe_unused]]void* ) override {
        WiFi.disconnect();
        WiFi.mode( WIFI_OFF );
        return RGH_OK;
    }

};
#ifndef RGH_INO_WIFI_DAEMON_NO_DECL
    inline WiFi_daemon WiFi_Daemon;
#endif

}