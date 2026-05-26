#pragma once
/**
 * @file: ucp/daemon.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */
#include <rgh/gep/daemon.hpp>

#ifdef RGH_TARGET_OS_FREERTOS
namespace rgh {

class Daemon_cluster_FreeRTOS : public Daemon_cluster {
public:
    struct config_t {
        int           iterate_interval_ms   = 5000;
        const char*   task_name             = "rgh/dmonclst";
        int           task_stack_depth      = 4096;
        UBaseType_t   task_priority         = configMAX_PRIORITIES - 1;
    };

_RGH_PROTECTED:
    config_t       _config     = {};
    TaskHandle_t   _tsk_main   = NULL;

_RGH_PROTECTED:
    bool _main_should_stop( void ) {
        return this->went_critical();
    }

_RGH_PROTECTED:
    static void _main( void* arg_ ) {
        auto* self = ( Daemon_cluster_FreeRTOS* )arg_;

    for(; not self->_main_should_stop() ;) {
        vTaskDelay( pdMS_TO_TICKS( self->_config.iterate_interval_ms ) );
        RGH_ASSERT_OR( RGH_OK == self->iterate_register() ) break;
    }
        vTaskDelete( NULL );
    }

public:
    status_t init( const config_t& config_ ) {
        _config = config_;

        RGH_ASSERT_OR( pdPASS == xTaskCreate(
            &Daemon_cluster_FreeRTOS::_main, _config.task_name, _config.task_stack_depth, this, _config.task_priority, &_tsk_main
        ) ) {
            return RGH_ERR_SYSCALL;
        }
        return RGH_OK;
    }

};

};
#endif