#pragma once
/**
 * @file: ucp/core.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/gep/core.hpp>

#ifndef HIGH
    #define HIGH 0x1
#elif 0x1 != HIGH
    #error "[RGH-UCP] - HIGH should be defined as 1."
#endif
#ifndef LOW
    #define LOW 0x0
#elif 0x0 != LOW
    #error "[RGH-UCP] - LOW should be defined as 0."
#endif

namespace rgh {

constexpr const char* const   Tag   = "[RGH]";

}

#ifdef RGH_TARGET_OS_FREERTOS
#include "freertos/FreeRTOS.h"

namespace rgh::freertos_literals {

unsigned long long operator "" _pdms2t( unsigned long long ms_ ) { return pdMS_TO_TICKS(ms_); }

};

namespace rgh::freertos {

class Dynamic_task {
public:
    typedef   std::function< void( const Dynamic_task* ) >    code_t;

public:
    Dynamic_task( 
        void 
    ) = default;

    Dynamic_task( 
        RGH_IN   const Dynamic_task& 
    ) = delete;

    Dynamic_task( 
        RGH_IN   Dynamic_task&& 
    ) = delete;

public:
    status_t launch(
        RGH_IN   const char*              id_,
        RGH_IN   configSTACK_DEPTH_TYPE   depth_,
        RGH_IN   UBaseType_t              prio_,
        RGH_IN   code_t                   code_
    ) {
        RGH_ASSERT_OR( _handle.load() == nullptr ) return RGH_ERR_WOULD_OVRWR;

        _code  = std::move( code_ ); 
        _flags = 0x0;

        TaskHandle_t handle = nullptr;
        RGH_ASSERT_OR( pdPASS == xTaskCreate( &_main, id_, depth_, this, prio_, &handle ) ) {
            _code = nullptr;
            return RGH_ERR_SYSCALL;
        }

        _handle.store( handle );
        return RGH_OK;
    }

    void wait_exit(
        void
    ) {
        const auto handle = _handle.load();
        if( handle == nullptr ) return;

        this->request_exit();
        _handle.wait( handle );
    }

_RGH_PROTECTED:
    std::atomic< TaskHandle_t >   _handle   = nullptr;
    code_t                        _code     = nullptr;
    mutable std::atomic_uint8_t   _flags    = { 0x0 };

_RGH_PROTECTED:
    static void _main( 
        RGH_IN   void*   arg_ 
    ) {
        auto* self = ( Dynamic_task* )arg_;
        RGH_ASSERT_OR( self && self->_code ) goto l_end;
    {
        for(; not self->exit_requested(); ) {
            self->_code( self );
            if( self->suspend_requested() ) vTaskSuspend( {} );
        }
    } 
    l_end:
        self->_handle.store( nullptr );
        self->_handle.notify_all();
        vTaskDelete( {} );
    }

public:
    RGH_inline bool running(
        void
    ) const { return _handle != nullptr; }
    RGH_inline operator bool ( 
        void 
    ) const { return this->running(); }

public:
    RGH_inline bool suspend_requested( void ) const { return _flags & BV(0); }
    RGH_inline bool exit_requested( void ) const { return _flags & BV(1); }
    RGH_inline bool should_return( void ) const { return _flags & ( BV(0) | BV(1) ); }

    RGH_inline void request_suspend( void ) const { SBV( _flags, 0 ); }
    RGH_inline void request_exit( void ) const { SBV( _flags, 1 ); }

    RGH_inline void resume( void ) const { RBV( _flags, 0 ); vTaskResume( _handle.load() ); }

};

};
#endif
