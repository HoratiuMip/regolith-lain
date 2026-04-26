#pragma once
/**
 * @file: osp/core.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/brp/descriptor.hpp>
#include <rgh/gep/core.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <csignal>
#include <cerrno>

#ifdef RGH_TARGET_OS_WINDOWS
    #define WINVER 0x0A00
    #include <winsock2.h>
    #include <ws2spi.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <wincodec.h>
    #include <Ws2bth.h>
    #include <BluetoothAPIs.h>
    #include <setupapi.h>
    #include <cfgmgr32.h>
    #include <devguid.h>
    #include <initguid.h>

#elifdef RGH_TARGET_OS_LINUX
    #include <sys/types.h>
    #include <sys/ioctl.h>
    
    #include <unistd.h>  
    
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/if_ether.h>
    #include <netinet/ip.h>
    #include <arpa/inet.h>
#endif

#define RGH_SPDLOG_PATTERN "[%n] [%^%l%$] %v"

namespace rgh {

enum LogComponent_ {
    LogComponent_General, LogComponent_IO, LogComponent_IMM, LogComponent_SCT, _LogComponent_COUNT
};

/* Auto generate these smh... */
#define RGH_LOGI(...) (rgh::_Internal.logger->info( __VA_ARGS__ ))
#define RGH_LOGW(...) (rgh::_Internal.logger->warn( __VA_ARGS__ ))
#define RGH_LOGE(...) (rgh::_Internal.logger->error( __VA_ARGS__ ))
#define RGH_LOGC(...) (rgh::_Internal.logger->critical( __VA_ARGS__ ))
#define RGH_LOGD(...) (rgh::_Internal.logger->debug( __VA_ARGS__ ))

#define RGH_SYS_ERR_MSG(s) (std::system_category().default_error_condition((s)).message())

#define RGH_LOGW_INT(s, f, ...) RGH_LOGW(f " [{}]" __VA_OPT__(,) __VA_ARGS__, RGH_STATUS_MSG(s))
#ifdef RGH_TARGET_OS_WINDOWS
#define RGH_LOGW_EX(s, f, ...) {const auto _rgh_es_ = GetLastError(); RGH_LOGW(f " [{}][{} - {}]" __VA_OPT__(,) __VA_ARGS__, RGH_STATUS_MSG(s), _rgh_es_, RGH_SYS_ERR_MSG(_rgh_es_) );}
#elifdef RGH_TARGET_OS_LINUX
#define RGH_LOGW_EX(s, f, ...) {const auto _rgh_es_ = errno; RGH_LOGW(f " [{}][{} - {}]" __VA_OPT__(,) __VA_ARGS__, RGH_STATUS_MSG(s), _rgh_es_, RGH_SYS_ERR_MSG(_rgh_es_) );}
#endif
#define RGH_LOGE_INT(s, f, ...) RGH_LOGE(f " [{}]" __VA_OPT__(,) __VA_ARGS__, RGH_STATUS_MSG(s))
#ifdef RGH_TARGET_OS_WINDOWS
#define RGH_LOGE_EX(s, f, ...) {const auto _rgh_es_ = GetLastError(); RGH_LOGE(f " [{}][{} - {}]" __VA_OPT__(,) __VA_ARGS__, RGH_STATUS_MSG(s), _rgh_es_, RGH_SYS_ERR_MSG(_rgh_es_) );}
#elifdef RGH_TARGET_OS_LINUX
#define RGH_LOGE_EX(s, f, ...) {const auto _rgh_es_ = errno; RGH_LOGE(f " [{}][{} - {}]" __VA_OPT__(,) __VA_ARGS__, RGH_STATUS_MSG(s), _rgh_es_, RGH_SYS_ERR_MSG(_rgh_es_) );}
#endif


#define RGH_LOGI_IO(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IO ]->info( __VA_ARGS__ ))
#define RGH_LOGW_IO(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IO ]->warn( __VA_ARGS__ ))
#define RGH_LOGE_IO(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IO ]->error( __VA_ARGS__ ))
#define RGH_LOGC_IO(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IO ]->critical( __VA_ARGS__ ))
#define RGH_LOGD_IO(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IO ]->debug( __VA_ARGS__ ))
#define RGH_LOGE_IO_INT(s, f, ...) RGH_LOGE(f _RGH_LOG_ERR_INT_FMT_STR __VA_OPT__(,) __VA_ARGS__, _RGH_LOG_ERR_INT_ARGS(s))
#define RGH_LOGE_IO_EX(s, f, ...) RGH_LOGE_IO(f _RGH_LOG_ERR_EX_FMT_STR __VA_OPT__(,) __VA_ARGS__, _RGH_LOG_ERR_EX_ARGS(s))

#define RGH_LOGI_IMM(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IMM ]->info( __VA_ARGS__ ))
#define RGH_LOGW_IMM(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IMM ]->warn( __VA_ARGS__ ))
#define RGH_LOGE_IMM(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IMM ]->error( __VA_ARGS__ ))
#define RGH_LOGC_IMM(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IMM ]->critical( __VA_ARGS__ ))
#define RGH_LOGD_IMM(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_IMM ]->debug( __VA_ARGS__ ))
#define RGH_LOGE_IMM_INT(s, f, ...) RGH_LOGE(f _RGH_LOG_ERR_INT_FMT_STR __VA_OPT__(,) __VA_ARGS__, _RGH_LOG_ERR_INT_ARGS(s))
#define RGH_LOGE_IMM_EX(s, f, ...) RGH_LOGE_IMM(f _RGH_LOG_ERR_EX_FMT_STR __VA_OPT__(,) __VA_ARGS__, _RGH_LOG_ERR_EX_ARGS(s))

#define RGH_LOGI_SCT(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_SCT ]->info( __VA_ARGS__ ))
#define RGH_LOGW_SCT(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_SCT ]->warn( __VA_ARGS__ ))
#define RGH_LOGE_SCT(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_SCT ]->error( __VA_ARGS__ ))
#define RGH_LOGC_SCT(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_SCT ]->critical( __VA_ARGS__ ))
#define RGH_LOGD_SCT(...) (rgh::_Internal._Component_loggers[ rgh::LogComponent_SCT ]->debug( __VA_ARGS__ ))
#define RGH_LOGE_SCT_INT(s, f, ...) RGH_LOGE(f _RGH_LOG_ERR_INT_FMT_STR __VA_OPT__(,) __VA_ARGS__, _RGH_LOG_ERR_INT_ARGS(s))
#define RGH_LOGE_SCT_EX(s, f, ...) RGH_LOGE_SCT(f _RGH_LOG_ERR_EX_FMT_STR __VA_OPT__(,) __VA_ARGS__, _RGH_LOG_ERR_EX_ARGS(s))

#define _RGH_LOG_ERR_INT_FMT_STR " [{}]"
#define _RGH_LOG_ERR_INT_ARGS( s ) RGH_STATUS_MSG(s)

#define _RGH_LOG_ERR_EX_FMT_STR " [{}] [{} - #{}]"
#ifdef RGH_TARGET_OS_WINDOWS
    #define _RGH_LOG_ERR_EX_ARGS( s ) RGH_STATUS_MSG(s), RGH_SYS_ERR_MSG(GetLastError()), GetLastError()
#elifdef RGH_TARGET_OS_LINUX
    #define _RGH_LOG_ERR_EX_ARGS( s ) RGH_STATUS_MSG(s), RGH_SYS_ERR_MSG(errno), errno
#endif


#define RGH_TXTUUID_FROM_THIS (std::format("rgh@{}",(void*)this).c_str())


enum InitFlags_ {
    InitFlags_None = 0x0,
    InitFlags_Sockets
};
struct init_args_t {
    int   flags   = InitFlags_None;
};
class _INTERNAL {
public:
    _INTERNAL( void ) {
    #define _MAKE_LOG_AND_PATERN( c, s ) _Component_loggers[ c ] = spdlog::stdout_color_mt( RGH_VERSION_STRING s ); _Component_loggers[ c ]->set_pattern( "[%^%l%$] [%Y-%m-%d %H:%M:%S] [%n] - %v" );   
        _MAKE_LOG_AND_PATERN( LogComponent_IO, "--I/O" );
        _MAKE_LOG_AND_PATERN( LogComponent_IMM, "--IMM" );
        _MAKE_LOG_AND_PATERN( LogComponent_SCT, "--SCT" );
    #undef _MAKE_LOG_AND_PATERN
        
        logger = spdlog::stdout_color_mt( RGH_VERSION_STRING ); 
        logger->set_pattern( RGH_SPDLOG_PATTERN );

        _Component_loggers[ LogComponent_General ] = logger;
    }

_RGH_PROTECTED:
    struct {
    #ifdef RGH_TARGET_OS_WINDOWS
        WSADATA   wsa_data;
    #endif

    } _Data;

public:
    status_t init( int argc_, char* argv_[], const init_args_t& args_ );

public:
    std::shared_ptr< spdlog::logger >   logger                                   = { nullptr };
    std::shared_ptr< spdlog::logger >   _Component_loggers[ _LogComponent_COUNT ] = { nullptr };

}; inline _INTERNAL _Internal;

RGH_inline status_t init( int argc_, char* argv_[], const init_args_t& args_ ) {
    return _Internal.init( argc_, argv_, args_ );
}

RGH_inline void set_log_level( LogComponent_ comp_, spdlog::level::level_enum lvl_ ) {
    _Internal._Component_loggers[ comp_ ]->set_level( lvl_ );
} 


struct bridge_t {
public:
    bridge_t( 
        RGH_IN   const std::string&   logger_name_ 
    ) {
        logger = spdlog::stdout_color_mt( logger_name_ ); 
        logger->set_pattern( RGH_SPDLOG_PATTERN );
    }

_RGH_PROTECTED:
    HVec< spdlog::logger >   logger   = nullptr;

public:
    RGH_inline spdlog::logger* operator -> ( void ) { return logger.get(); }
};


struct on_scope_exit_c_t {
    typedef   void(*proc_t)(void*);
    on_scope_exit_c_t( proc_t proc_, void* arg_ = NULL ) : _proc{ ( proc_t&& )proc_ }, _arg{ arg_ } {}
    proc_t   _proc;
    void*    _arg;
    ~on_scope_exit_c_t() { if( nullptr != _proc ) _proc( _arg ); } 
    RGH_inline void drop( void ) { _proc = nullptr; }
};
#define RGH_ON_SCOPE_EXIT_C( proc, arg ) on_scope_exit_c_t _on_scope_exit_{ proc, arg };

struct on_scope_exit_l_t {
    typedef   std::function< void() >   proc_t;
    on_scope_exit_l_t( proc_t proc_ ) : _proc{ ( proc_t&& )proc_ } {}
    proc_t   _proc;
    ~on_scope_exit_l_t() { if( nullptr != _proc ) _proc(); } 
    RGH_inline void drop( void ) { _proc = nullptr; }
};
#define RGH_ON_SCOPE_EXIT_L( proc ) on_scope_exit_l_t _on_scope_exit_{ proc };

#define RGH_ON_SCOPE_EXIT_DROP _on_scope_exit_.drop()

#define RGH_UNREACHABLE std::unreachable()

};



