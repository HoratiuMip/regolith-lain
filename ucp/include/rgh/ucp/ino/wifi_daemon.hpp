#pragma once
/**
 * @file: gep/ino/wifi_daemon.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */
#include <WiFi.h>

#include <rgh/brp/IO_string_utils.hpp>
#include <rgh/ucp/daemon.hpp>

namespace rgh::ino {

struct wifi_daemon_start_args_t {
    std::string   ssid      = {};
    std::string   pwrd      = {};
    uint16_t      vrf_ms    = 1'000;
    uint8_t       vrf_cnt   = 3;
};

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
            this->ssid(), this->connected(), crt_rssi, ( const char* )io::wifi_rssi_str_t{ crt_rssi },
            WiFi.localIP().toString().c_str(), WiFi.channel()
        );
    }

public:
    RGH_inline bool connected( void ) const { return WiFi.status() == WL_CONNECTED; }

    RGH_inline int32_t rssi( void ) const { return WiFi.RSSI(); }
    RGH_inline std::string ssid( void ) const { return WiFi.SSID().c_str(); }

public:
    struct Dridge {
        virtual wifi_daemon_start_args_t get_start_args( void ) const = 0;
    };

_RGH_PROTECTED:
    virtual status_t _daemon_start( void* arg_ ) override {
        ESP_LOGI( Tag, "WiFi: starting..." );

        RGH_ASSERT_OR( arg_ ) { ESP_LOGE( Tag, "WiFi: bad dridge." ); return RGH_ERR_BADARG; }

        auto [
            ssid, pwrd,
            vrf_ms, vrf_cnt
        ] = (( const Dridge* )arg_)->get_start_args();

        RGH_ASSERT_OR( not ssid.empty() and not pwrd.empty() ) { ESP_LOGE( Tag, "WiFi: bad creds." ); return RGH_ERR_NOT_FOUND; }

        WiFi.mode( WIFI_STA );
        WiFi.setAutoReconnect( true );
        WiFi.begin( ssid.c_str(), pwrd.c_str() );

        uint16_t attempt = 0;
        do {
            vTaskDelay( pdMS_TO_TICKS( vrf_ms ) );
            RGH_ASSERT_OR( ++attempt < vrf_cnt ) {
                ESP_LOGE( Tag, "WiFi: could not connect: %s", ssid.c_str() );
                return RGH_ERR_PLATFORMCALL;
            }
        } while( not this->connected() );

        ESP_LOGI( Tag, "WiFi: connected: %s.", ssid.c_str() );
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

#define RGH_INO_WIFI_DAEMON_QUICK_DRIDGE( decl_name_, get_start_args_ )                                        \
    struct _rgh_ino_wifi_daemon_dridge_##decl_name_ : WiFi_daemon::Dridge {                                    \
        virtual wifi_daemon_start_args_t get_start_args( void ) const override { return (get_start_args_)(); } \
    } decl_name_;

}