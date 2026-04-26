/**
 * @file: OSp/IO_utils.cpp
 * @brief: Implementation file.
 * @details: -
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/IO_utils.hpp>

namespace rgh::io {

static status_t _populate_ports( COM_Ports::container_t& ports_, int* count_ = nullptr ) {
#ifdef RGH_TARGET_OS_WINDOWS
    HDEVINFO dev_set = SetupDiGetClassDevs( &GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT );
    RGH_ASSERT_OR( dev_set != INVALID_HANDLE_VALUE ) return RGH_ERR_SYSCALL;

    SP_DEVINFO_DATA dev_data{
        .cbSize = sizeof( SP_DEVINFO_DATA )
    };
    int  count = 0;
    char buffer[ 256 ];

    ports_.clear();

    for( int n = 0; SetupDiEnumDeviceInfo( dev_set, n, &dev_data ); ++n ) {
        if( !SetupDiGetDeviceRegistryPropertyA( dev_set, &dev_data, SPDRP_FRIENDLYNAME, NULL, ( PBYTE )buffer, sizeof( buffer ), NULL ) ) continue;
        ++count;
        auto& port = ports_.emplace_back( COM_port_t{ 
            .id       = "COM", 
            .friendly = buffer
        } );
        
        char* last_occ = nullptr;
        char* ptr      = nullptr;
        while( (ptr = strstr( last_occ ? last_occ : buffer, "COM" )) && last_occ < buffer + sizeof( buffer ) ) last_occ = ptr += 0x3;
        
        if( last_occ ) while( *last_occ >= '0' && *last_occ <= '9' && last_occ < buffer + sizeof( buffer ) ) port.id += *( last_occ++ );
    }

    SetupDiDestroyDeviceInfoList( dev_set );
    if( count_ ) *count_ = count;
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
            
    auto* that = ( COM_Ports* )that_;

    that->refresh();

    return 0x0;
}
#endif

RGH_IMPL_FNC COM_Ports& COM_Ports::refresh( void ) {
    auto     ports      = this->control();
    int      port_count = 0;
    status_t status     = _populate_ports( *ports, &port_count );

    RGH_ASSERT_OR( RGH_OK == status ) {
        if( _config.clear_container_on_failed_refresh ) { ports->clear(); goto l_fail_no_err; }
        
        ports.drop();
        RGH_LOGE_IO_EX( status, "Bad COM ports refresh." );
        return *this;
    }
l_fail_no_err:

    if( not _refresh_cb_map.empty() ) {
        std::lock_guard lck{ _refresh_cb_mtx };
        for( auto [ _, cb ] : _refresh_cb_map ) cb( *ports );
    }
    ports.commit();

    if( port_count > 0x0 ) RGH_LOGI_IO( "COM ports refreshed, found [{}] port(s).", port_count );
    else                   RGH_LOGI_IO( "COM ports refreshed, no ports found." );
    return *this;
}

RGH_IMPL_FNC status_t COM_Ports::register_listen( void ) {
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

RGH_IMPL_FNC status_t COM_Ports::unregister_listen( void ) {
#ifdef RGH_TARGET_OS_WINDOWS
    return CM_Unregister_Notification( _notif ) == CR_SUCCESS ? 0x0 : -0x1;
#else
    return RGH_ERR_NOT_IMPL;
#endif
}

RGH_IMPL_FNC status_t COM_Ports::register_refresh_callback( const char* key_, refresh_cb_t cb_ ) {
    std::lock_guard lck{ _refresh_cb_mtx };
    auto [ itr, already ] = _refresh_cb_map.insert( std::make_pair< std::string, refresh_cb_t >( key_, std::move( cb_ ) ) );
    if( already && not _config.allow_refresh_callback_overwrite ) return RGH_ERR_WOULD_OVRWR;

    itr->second = cb_;
    return RGH_OK;
}

RGH_IMPL_FNC status_t COM_Ports::unregister_refresh_callback( const char* key_ ) {
    std::lock_guard lck{ _refresh_cb_mtx };
    _refresh_cb_map.erase( key_ );
    return RGH_OK;
}

}