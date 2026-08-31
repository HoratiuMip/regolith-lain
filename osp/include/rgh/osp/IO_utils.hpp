#pragma once
/**
 * @file: osp/IO_utils.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */
#include <rgh/brp/IO_port.hpp>
#include <rgh/brp/IO_utils.hpp>
#include <rgh/gep/dispenser.hpp>
#include <rgh/osp/core.hpp>

#include <expected>

namespace rgh::io {

struct COM_port_t {
    std::string   id;
    std::string   detail;
};

#define COM_PORT_FILTER_ int
enum _COM_PORT_FILTER_enum : int {
    COM_PORT_FILTER_CDC   = BV( 0x0 ),
    COM_PORT_FILTER_VIDEO = BV( 0x1 )
};

struct COM_ports_config_t {
    COM_PORT_FILTER_   filter                             = COM_PORT_FILTER_CDC;
    bool               allow_hotplug_callback_overwrite   = false;
    bool               clear_container_on_failed_scan     = true;
};

struct COM_ports_init_args_t {
    bool                 scan     = true;
    bool                 listen   = false;
    COM_ports_config_t   config   = {};
};

class COM_ports : public Dispenser< std::vector< COM_port_t > > {
public:
    using container_t  = std::vector< COM_port_t >;
    using hotplug_cb_t = std::function< void( container_t& ) >; 

public:
    COM_ports( const DispenserMode_ disp_mode_, const COM_ports_init_args_t& args_ ) 
    : Dispenser{ disp_mode_ }, _config{ std::move( args_.config ) }
    {
        if( args_.scan ) this->scan();
        if( args_.listen ) this->register_listen();
    }

_RGH_PROTECTED:
#ifdef RGH_TARGET_OS_WINDOWS
    HCMNOTIFICATION                         _notif            = nullptr;
#elifdef RGH_TARGET_OS_LINUX

#endif
    std::map< std::string, hotplug_cb_t >   _hotplug_cb_map   = {};
    std::mutex                              _hotplug_cb_mtx   = {};
    COM_ports_config_t                      _config           = {};

public:
    /**
     * @brief: Rescan the existing COM ports and push them in the underlying container.
     */
    COM_ports& scan();

public:
    /**
     * @brief: Start listening for hotplug events over the COM ports.
     */
    status_t register_listen( void );
    /**
     * @brief: Stop listening for hotplug events over the COM ports.
     */
    status_t unregister_listen( void );

public:
    status_t register_hotplug_callback( const char* key_, hotplug_cb_t cb_ );
    status_t unregister_hotplug_callback( const char* key_ );

};


std::expected< std::vector< ipv4_addr_t >, ret_t > ipv4_hosts_of( std::string_view domain_ ) noexcept; 


std::expected< ntp_packet_t, ret_t > ntp_get( Port& port_, bool make_unix_ = true ) noexcept; 

}