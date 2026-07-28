include( FetchContent )
include( ExternalProject )

if( POLICY CMP0169 )
    cmake_policy( SET CMP0169 OLD )
endif()

macro( RGH_Fnc_JustCloneRepo url tag alias excom_dir )
    message( STATUS "[RGH] Cloning \"${url}\", tag \"${tag}\" to \"${excom_dir}/${alias}\"." )
    FetchContent_Declare(
        ${alias}
        GIT_REPOSITORY "${url}"
        SOURCE_DIR     "${excom_dir}/${alias}"
        GIT_TAG        "${tag}"
    )
    FetchContent_Populate( ${alias} )
endmacro( RGH_Fnc_JustCloneRepo )

macro( RGH_Fnc_CloneRepo url tag alias excom_dir )
    message( STATUS "[RGH] Cloning \"${url}\", tag \"${tag}\" to \"${excom_dir}/${alias}\"." )
    FetchContent_Declare(
        ${alias}
        GIT_REPOSITORY "${url}"
        SOURCE_DIR     "${excom_dir}/${alias}"
        GIT_TAG        "${tag}"
    )
    FetchContent_MakeAvailable( ${alias} )
endmacro( RGH_Fnc_CloneRepo )