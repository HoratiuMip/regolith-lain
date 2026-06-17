#pragma once
/**
 * @file: gep/wifi_daemon.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/brp/IO_port.hpp>
#include <rgh/ucp/daemon.hpp>

#include <WiFiClientSecure.h>
#ifdef RGH_INO_THINGSBOARD_DAEMON_MQTT_MAX_PACKET_SIZE_
    #define MQTT_MAX_PACKET_SIZE RGH_INO_THINGSBOARD_DAEMON_MQTT_MAX_PACKET_SIZE_
#endif
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>

namespace rgh::ino {

template< typename T_ >
concept Thingsboard_daemon_static_cfg = requires {
    (uint32_t)T_::MAX_JSON_FIELDS_PER_RECV;

    (uint32_t)T_::RPC_ARRAY_SIZE;
    (uint32_t)T_::MAX_JSON_FIELDS_PER_SEND;

    (uint32_t)T_::SATTR_ARRAY_SIZE;
    (uint32_t)T_::SATTR_REQ_TIMEOUT_MS;
};
struct thingsboard_daemon_static_default_cfg_t {
    inline static constexpr uint32_t   MAX_JSON_FIELDS_PER_RECV   = Default_Response_Amount;
    inline static constexpr uint32_t   RPC_ARRAY_SIZE             = Default_Subscriptions_Amount;
    inline static constexpr uint32_t   MAX_JSON_FIELDS_PER_SEND   = Default_RPC_Amount;
    inline static constexpr uint32_t   SATTR_ARRAY_SIZE           = Default_Attributes_Amount;
    inline static constexpr uint32_t   SATTR_REQ_TIMEOUT_MS       = 3'000;
};

typedef void( *thingsboard_daemon_sattr_cb_t )( const JsonObjectConst& );
typedef std::function< void( void ) > thingsboard_daemon_loop_cb_t;

struct thingsboard_daemon_start_args_t {
    const char*                     server        = nullptr;
    const char*                     token         = nullptr;
    io::ipv4_port_t                 port          = 0x0;

	std::vector< RPC_Callback >     rpc_list      = {};
	std::vector< const char* >      sattr_list    = {};
	thingsboard_daemon_sattr_cb_t   sattr_cb      = nullptr;

	thingsboard_daemon_loop_cb_t    loop_cb       = nullptr;
    UBaseType_t                     loop_prio     = configMAX_PRIORITIES - 1;
    uint32_t                        loop_int_ms   = 1'000;
};

template< Thingsboard_daemon_static_cfg CFG_ = thingsboard_daemon_static_default_cfg_t >
class Thingsboard_daemon : public Daemon {
public:
    virtual std::string_view daemon_name( void ) const override { return "Thingsboard"; }
    virtual std::string daemon_report( [[maybe_unused]]void* ) const override {
        return std::format( 
            "=-. Thingsboard .-=\n"
            "\t- connected: {}\n"
            , 
            this->connected()
        );
    }

public:
    RGH_inline bool connected( void ) const { return const_cast< decltype(_dev)& >( _dev ).connected(); }

public:
    struct Dridge {
        virtual thingsboard_daemon_start_args_t get_start_args( void ) const = 0;
    };

_RGH_PROTECTED:
    WiFiClientSecure                                                          _wifi_client     = {};
	Arduino_MQTT_Client                                                       _mqtt_client     = { _wifi_client };

    freertos::Dynamic_task                                                    _tsk_main        = {};

    Server_Side_RPC< CFG_::RPC_ARRAY_SIZE, CFG_::MAX_JSON_FIELDS_PER_SEND >   _rpc             = {};
    Attribute_Request< 1, CFG_::SATTR_ARRAY_SIZE >                            _sattr_request   = {};
    Shared_Attribute_Update< 1, CFG_::SATTR_ARRAY_SIZE >                      _sattr_update    = {};

    const std::array< IAPI_Implementation*, 3 >                               _APIs            = {
        &_rpc, &_sattr_request, &_sattr_update
    };

    ThingsBoardSized< CFG_::MAX_JSON_FIELDS_PER_RECV >                        _dev             = { _mqtt_client, 1024, 8192, _APIs };

    thingsboard_daemon_loop_cb_t                                              _loop_cb         = nullptr;  
    uint32_t                                                                  _loop_int_ms     = 200;    

_RGH_PROTECTED:
    status_t _daemon_start( void* arg_ ) override {
        ESP_LOGI( Tag, "thingsboard: starting..." );

        RGH_ASSERT_OR( arg_ ) { ESP_LOGE( Tag, "thingsboard: bad dridge." ); return RGH_ERR_BADARG; }
        auto [
            server, token, port,
            rpc_list, sattr_list, sattr_cb,
            loop_cb, loop_prio, loop_int_ms
        ] = (( const Dridge* )arg_)->get_start_args();

        uint8_t cred_bits = ( (bool)server << 2 )    |
                            ( ( port != 0x0 ) << 1 ) |
                            ( (bool)token );
        RGH_ASSERT_OR( cred_bits == 0b111 ) { 
            ESP_LOGE( Tag, "thingsboard: credentials missing: %d.", cred_bits );
            return RGH_ERR_NOT_FOUND;
        }
        
        _wifi_client.setInsecure();
        RGH_ASSERT_OR( _dev.connect( server, token, port ) ) {
            ESP_LOGE( Tag, "thingsboard: bad connection." );
            return RGH_ERR_EXCOMCALL;
        } 
        
        if( not rpc_list.empty() ) {
            RGH_ASSERT_OR( _rpc.RPC_Subscribe( rpc_list.cbegin(), rpc_list.cend() ) ) {
                ESP_LOGE( Tag, "thingsboard: bad rpc subscribe." );
                return RGH_ERR_EXCOMCALL;
            } else {
                ESP_LOGI( Tag, "thingsboard: subscribed %d rpc(s).", (int)rpc_list.size() );
            }
        }
        
        if( not sattr_list.empty() and sattr_cb ) {
            RGH_ASSERT_OR( _sattr_update.Shared_Attributes_Subscribe(
                Shared_Attribute_Callback< CFG_::SATTR_ARRAY_SIZE >{ sattr_cb, sattr_list.cbegin(), sattr_list.cend() }
            ) ) {
                ESP_LOGE( Tag, "thingsboard: bad shared subscribe." );
                return RGH_ERR_EXCOMCALL;
            }

            RGH_ASSERT_OR( _sattr_request.Shared_Attributes_Request(  
                Attribute_Request_Callback< CFG_::SATTR_ARRAY_SIZE >{ sattr_cb, CFG_::SATTR_REQ_TIMEOUT_MS*1000, [](){}, sattr_list }
            ) ) {
                ESP_LOGE( Tag, "thingsboard: bad shared request." );
                return RGH_ERR_EXCOMCALL;
            }
        }

        _loop_cb     = std::move( loop_cb );
        _loop_int_ms = loop_int_ms;

        RGH_ASSERT_STATUS_OR( _tsk_main.launch(
            "rgh-tbd", 8192, loop_prio, [ this ] ( const freertos::Dynamic_task* task_ ) -> void {
                for(; not task_->should_return() and this->daemon_is_positive(); ) {
                    vTaskDelay( pdMS_TO_TICKS( _loop_int_ms ) );

                    RGH_ASSERT_AND( this->connected() ) {
                        if( _loop_cb ) this->_loop_cb();
                        _dev.loop();
                    }
                }
            }
        ) ) {
            ESP_LOGE( Tag, "thingsboard: bad main task create." );
            return RGH_ERR_SYSCALL;
        }
       
        ESP_LOGI( Tag, "thingsboard: started." );
        return OK;
    }

    virtual status_t _daemon_stop( [[maybe_unused]]void* ) override {
        _tsk_main.wait_exit();

        _dev.disconnect();
        _dev.Cleanup_Subscriptions();

        return RGH_OK;
    }

};
#ifdef RGH_INO_THINGSBOARD_DAEMON_CFG_
    inline Thingsboard_daemon< RGH_INO_THINGSBOARD_DAEMON_CFG_ > Thingsboard_Daemon;
#endif

#define RGH_INO_THINGSBOARD_DAEMON_QUICK_DRIDGE( decl_name_, get_start_args_ )                                         \
    struct _rgh_ino_thingsboard_daemon_dridge_##decl_name_ : Thingsboard_daemon<>::Dridge {                            \
        virtual thingsboard_daemon_start_args_t get_start_args( void ) const override { return (get_start_args_)(); }  \
    } decl_name_;

}