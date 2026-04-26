/**
 * @file: osp/core.cpp
 * @brief: Implementation file.
 * @details: -
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/core.hpp>

namespace rgh {


RGH_IMPL_FNC status_t _INTERNAL::init( int argc_, char* argv_[], const init_args_t& args_ ) {    
    RGH_LOGI( "Hello there from RGH, version {}.{}.{}. Initializing ...", RGH_VERSION_MAJOR, RGH_VERSION_MINOR, RGH_VERSION_PATCH );

    int warn_count = 0x0;

    if( args_.flags & InitFlags_Sockets ) {
        RGH_LOGD( "Initializing IO sockets..." );

    #ifdef RGH_TARGET_OS_WINDOWS
        int result = WSAStartup( 0x0202, &_Data.wsa_data );
        RGH_ASSERT_OR( 0x0 == result ) {
            RGH_LOGE( "Bad WSA startup, [{}].", result );
            ++warn_count;
            goto l_bad_end;
        }

    #endif

        RGH_LOGI( "Initialization of input/output sockets completed." ); 
        goto l_end;
    l_bad_end:
        RGH_LOGW( "Flawed initialization of input/output sockets." );
    l_end:
    }

    if( 0x0 == warn_count ) RGH_LOGI( "Initialization of the operating system plate completed flawlessly." );
    else RGH_LOGW( "Initialization of the operating system plate completed with {} warnings.", warn_count );
    return 0x0;
}


};
