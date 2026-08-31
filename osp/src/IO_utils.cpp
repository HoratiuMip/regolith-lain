/**
 * @file: OSp/IO_utils.cpp
 * @brief: Implementation file.
 * @details: -
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/IO_utils.hpp>

#ifdef RGH_TARGET_OS_WINDOWS

#elifdef RGH_TARGET_OS_LINUX
    #include <libudev.h>

    #include <netdb.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    
    #include <expected>
#endif

namespace rgh::io {

static status_t _populate_ports( COM_ports::container_t& ports_, COM_PORT_FILTER_ filter_ ) {
    ports_.clear();

#ifdef RGH_TARGET_OS_WINDOWS
    HDEVINFO dev_set = SetupDiGetClassDevs( &GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT );
    RGH_ASSERT_OR( dev_set != INVALID_HANDLE_VALUE ) return RGH_ERR_SYSCALL;

    SP_DEVINFO_DATA dev_data{
        .cbSize = sizeof( SP_DEVINFO_DATA )
    };
    char buffer[ 256 ];

    for( int n = 0; SetupDiEnumDeviceInfo( dev_set, n, &dev_data ); ++n ) {
        if( !SetupDiGetDeviceRegistryPropertyA( dev_set, &dev_data, SPDRP_FRIENDLYNAME, NULL, ( PBYTE )buffer, sizeof( buffer ), NULL ) ) continue;

        auto& port = ports_.emplace_back( COM_port_t{ 
            .id     = "COM", 
            .detail = buffer
        } );
        
        char* last_occ = nullptr;
        char* ptr      = nullptr;
        while( (ptr = strstr( last_occ ? last_occ : buffer, "COM" )) && last_occ < buffer + sizeof( buffer ) ) last_occ = ptr += 0x3;
        
        if( last_occ ) while( *last_occ >= '0' && *last_occ <= '9' && last_occ < buffer + sizeof( buffer ) ) port.id += *( last_occ++ );
    }

    SetupDiDestroyDeviceInfoList( dev_set );
    return RGH_OK;

#elifdef RGH_TARGET_OS_LINUX
    udev* const udv = udev_new();
    RGH_ASSERT_OR( udv ) return RGH_ERR_SYSCALL;

    udev_enumerate* const enumerate = udev_enumerate_new( udv );
    RGH_ASSERT_OR( enumerate ) {
        udev_unref( udv );
        return RGH_ERR_SYSCALL;
    }

    if( filter_ & COM_PORT_FILTER_CDC ) udev_enumerate_add_match_subsystem( enumerate, "tty" );
    if( filter_ & COM_PORT_FILTER_VIDEO ) udev_enumerate_add_match_subsystem( enumerate, "video4linux" );
    udev_enumerate_scan_devices( enumerate );

    udev_list_entry* devices = udev_enumerate_get_list_entry( enumerate );
    udev_list_entry* entry   = nullptr;

    udev_list_entry_foreach( entry, devices ) {
        const char* sys_path = udev_list_entry_get_name( entry );

        udev_device* dev = udev_device_new_from_syspath( udv, sys_path );
        RGH_ASSERT_OR( dev ) continue;
        RGH_ON_SCOPE_EXIT_L( [ &dev ] ( void ) -> void { udev_device_unref( dev ); } );

        udev_device* parent = udev_device_get_parent( dev ); RGH_ASSERT_OR( parent ) continue;
        const char* dev_node = udev_device_get_devnode( dev ); RGH_ASSERT_OR( dev_node ) continue;
        
        std::string dev_path = { dev_node };
        RGH_ASSERT_OR( dev_path.find( "ttyS" ) == std::string::npos ) continue;

        const char* parent_subs = udev_device_get_subsystem( parent );
        bool is_usb = parent_subs && ( strcmp( parent_subs, "usb" ) == 0x0 || strcmp( parent_subs, "usb-serial" ) == 0x0 );
        if( !is_usb ) {
            is_usb = dev_path.find( "ttyUSB" ) != std::string::npos || 
                     dev_path.find( "ttyACM" ) != std::string::npos;
        }
        RGH_ASSERT_OR( is_usb ) continue;

        const char* ven = udev_device_get_property_value( dev, "ID_VENDOR_FROM_DATABASE" ) ?:
                          udev_device_get_property_value( dev, "ID_VENDOR" )               ?:
                          RGH_NA;
        
        const char* mdl = udev_device_get_property_value( dev, "ID_MODEL_FROM_DATABASE" ) ?:
                          udev_device_get_property_value( dev, "ID_MODEL" )               ?:
                          RGH_NA;
        
        const char* vid  = udev_device_get_property_value( dev, "ID_VENDOR_ID" ) ?: RGH_NA;
        const char* mid  = udev_device_get_property_value( dev, "ID_MODEL_ID" ) ?: RGH_NA;
 
        std::string detail = std::format( "{} - {} - ({}:{})", mdl, ven, vid, mid );
        RGH_ASSERT_OR( ports_.end() == std::ranges::find_if( ports_, [ &detail ] ( const COM_port_t& port_ ) -> bool {
            return port_.detail == detail;
        } ) ) continue;

        ports_.emplace_back( COM_port_t{
            .id = std::move( dev_path ),
            .detail = std::move( detail )
        } );
    }

    udev_enumerate_unref( enumerate );
    udev_unref( udv );

    return RGH_OK;

#else
    return RGH_ERR_NOT_IMPL;
#endif
}

#ifdef RGH_TARGET_OS_WINDOWS
static DWORD CALLBACK _listen_callback (
    [[maybe_unused]]HCMNOTIFICATION,
    PVOID                            that_,
    CM_NOTIFY_ACTION                 action_,
    PCM_NOTIFY_EVENT_DATA            event_,
    [[maybe_unused]]DWORD
) {
    if( action_ != CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL && action_ != CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL ) return 0x0;

    if( event_->FilterType != CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE ) return 0x0;

    if( not IsEqualGUID( event_->u.DeviceInterface.ClassGuid, GUID_DEVINTERFACE_COMPORT ) ) return 0x0;
            
    auto* that = ( COM_ports* )that_;
    that->scan();

    return 0x0;
}
#endif

RGH_IMPL_FNC COM_ports& COM_ports::scan( void ) {
    auto     ports  = this->control();
    status_t status = _populate_ports( *ports, _config.filter );

    RGH_ASSERT_OR( RGH_OK == status ) {
        if( _config.clear_container_on_failed_scan ) { ports->clear(); goto l_fail_no_err; }
        
        ports.drop();
        RGH_BRDG_LOGE( "com ports: bad scan." );
        return *this;
    }
l_fail_no_err:

    if( not _hotplug_cb_map.empty() ) {
        std::lock_guard lck{ _hotplug_cb_mtx };
        for( auto [ _, cb ] : _hotplug_cb_map ) cb( *ports );
    }
    auto port_count = ports->size();
    ports.commit();

    if( port_count > 0x0 ) RGH_BRDG_LOGI( "com ports: found {} port(s).", port_count );
    else                   RGH_BRDG_LOGI( "com ports: no ports found." );
    return *this;
}

RGH_IMPL_FNC status_t COM_ports::register_listen( void ) {
#ifdef RGH_TARGET_OS_WINDOWS
    CM_NOTIFY_FILTER filter {
        .cbSize                         = sizeof( CM_NOTIFY_FILTER ),
        .FilterType                     = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE,
        .u{.DeviceInterface={.ClassGuid = GUID_DEVINTERFACE_COMPORT}}
    };

    return CM_Register_Notification( &filter, ( PVOID )this, &_listen_callback, &_notif ) == CR_SUCCESS ? 0x0 : -0x1;
#else
    return RGH_ERR_NOT_IMPL;
#endif
}

RGH_IMPL_FNC status_t COM_ports::unregister_listen( void ) {
#ifdef RGH_TARGET_OS_WINDOWS
    return CM_Unregister_Notification( _notif ) == CR_SUCCESS ? 0x0 : -0x1;
#else
    return RGH_ERR_NOT_IMPL;
#endif
}

RGH_IMPL_FNC status_t COM_ports::register_hotplug_callback( const char* key_, hotplug_cb_t cb_ ) {
    std::lock_guard lck{ _hotplug_cb_mtx };
    auto [ itr, already ] = _hotplug_cb_map.insert( std::make_pair< std::string, hotplug_cb_t >( key_, std::move( cb_ ) ) );
    if( already && not _config.allow_hotplug_callback_overwrite ) return RGH_ERR_WOULD_OVRWR;

    itr->second = cb_;
    return RGH_OK;
}

RGH_IMPL_FNC status_t COM_ports::unregister_hotplug_callback( const char* key_ ) {
    std::lock_guard lck{ _hotplug_cb_mtx };
    _hotplug_cb_map.erase( key_ );
    return RGH_OK;
}


//# A convenient wrapper over getaddrinfo(). See https://man7.org/linux/man-pages/man3/getaddrinfo.3.html.
RGH_IMPL_FNC std::expected< std::vector< ipv4_addr_t >, ret_t > ipv4_hosts_of( std::string_view domain_ ) noexcept {
    RGH_ASSERT_OR( not domain_.empty() ) {
        RGH_BRDG_LOGE( "ipv4 hosts of: no domain provided." );
        return std::unexpected{ RGH_ERR_BADARG };
    }
#if defined( RGH_TARGET_OS_LINUX )
    addrinfo hints{
        .ai_family   = AF_INET
    };  

    addrinfo* result;
    RGH_ASSERT_OR_EX( getaddrinfo( domain_.cbegin(), nullptr, &hints, &result ), 0 == ret_ ) {
        RGH_BRDG_LOGE( "ipv4 hosts of: {}", gai_strerror( ret_ ) );
        return std::unexpected{ ret_ };
    }

    std::vector< ipv4_addr_t > hosts;
    for( addrinfo* rp = result; rp != nullptr; rp = rp->ai_next ) {
        const ipv4_addr_t entry = static_cast< ipv4_addr_t >( ntohl( (( sockaddr_in* )( rp->ai_addr ))->sin_addr.s_addr ) );
        RGH_ASSERT_OR( std::ranges::find( hosts, entry ) == hosts.end() ) continue;
        hosts.emplace_back( entry );
    }

    freeaddrinfo( result );
    return hosts;
#else
    return std::unexpected{ RGH_ERR_NOT_IMPL };
#endif
}


//# Request an NTP packet on the given port.
RGH_IMPL_FNC std::expected< ntp_packet_t, ret_t > ntp_get( Port& port_, bool make_unix_  ) noexcept {
    auto packet = ntp_packet_t::client_request();

    RGH_ASSERT_STATUS_OR( port_.write( { 
        .src_ptr = reinterpret_cast< rgh::byte_t* >( &packet ), 
        .src_n   = sizeof( packet ),
        .req_all = true
    } ) ) return std::unexpected{ status_ };

    RGH_ASSERT_STATUS_OR( port_.read( { 
        .dst_ptr = reinterpret_cast< rgh::byte_t* >( &packet ), 
        .dst_n   = sizeof( packet ),
        .req_all = true
    } ) ) return std::unexpected{ status_ };

#define _RGH_IO_NTP_GET_CVT_END( fld_ ) packet.fld_ = ntohl( packet.fld_ );
    _RGH_IO_NTP_GET_CVT_END( root_delay );
    _RGH_IO_NTP_GET_CVT_END( root_dispersion );
    _RGH_IO_NTP_GET_CVT_END( ref_id );
    _RGH_IO_NTP_GET_CVT_END( ts.ref_s );
    _RGH_IO_NTP_GET_CVT_END( ts.ref_f );
    _RGH_IO_NTP_GET_CVT_END( ts.org_s );
    _RGH_IO_NTP_GET_CVT_END( ts.org_f );
    _RGH_IO_NTP_GET_CVT_END( ts.rx_s );
    _RGH_IO_NTP_GET_CVT_END( ts.rx_f );
    _RGH_IO_NTP_GET_CVT_END( ts.tx_s ); 
    _RGH_IO_NTP_GET_CVT_END( ts.tx_f ); 
#undef _RGH_IO_NTP_GET_CVT_END

    if( make_unix_ ) packet.make_unix();

    return packet;
}

}