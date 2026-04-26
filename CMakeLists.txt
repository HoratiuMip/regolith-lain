cmake_minimum_required( VERSION 3.28.0 )

message( STATUS "[RGH] CMake begin..." )

project(
    RGH
    VERSION 1.0.0
    LANGUAGES C CXX
)

include( CheckLanguage )
check_language( Fortran )
if( CMAKE_Fortran_COMPILER )
    enable_language( Fortran )
    message( STATUS "[RGH] Fortran found." )
endif()

set( RGH_VERSION_MAJOR 1 )
set( RGH_VERSION_MINOR 0 )
set( RGH_VERSION_PATCH 3 )
set( RGH_VERSION_STRING "rghv1.0.3" )

set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_EXTENSIONS ON )

set( RGH_GENERATED_FILE_CAUTION_MSG "/*\n[RGH] CAUTION!\nTHIS FILE WAS GENERATED DURING BUILD AND IT WILL BE OVERRIDEN IN THE NEXT ONE.\nDO NOT MODIFY AS THE MODIFICATIONS WILL BE LOST.\n*/")

option( RGH_Opt_TargetPlate_OS OFF )
option( RGH_Opt_TargetPlate_uC OFF )

option( RGH_Opt_TargetOS_Windows OFF )
option( RGH_Opt_TargetOS_Linux OFF )
option( RGH_Opt_TargetOS_FreeRTOS OFF )

if( NOT DEFINED RGH_TARGET_PLATE )
    message( ERROR "[RGH] Target plate not specified. Aborted." )
    return()
endif()

if( RGH_TARGET_PLATE STREQUAL "OS" )
    set( RGH_Opt_TargetPlate_OS ON )

    if( NOT DEFINED RGH_TARGET_OS )
        message( ERROR "[RGH] Target operating system not specified. Aborted." )
        return()
    endif()
elseif( RGH_TARGET_PLATE STREQUAL "uC" )
    set( RGH_Opt_TargetPlate_uC ON )
else()
    message( ERROR "[RGH] Target plate \"${RGH_TARGET_PLATE}\" is not valid." )
    return()
endif()

if( RGH_TARGET_OS STREQUAL "Windows" )
    set( RGH_Opt_TargetOS_Windows ON )
elseif( RGH_TARGET_OS STREQUAL "Linux" )
    set( RGH_Opt_TargetOS_Linux ON )
elseif( RGH_TARGET_OS STREQUAL "FreeRTOS" )
    set( RGH_Opt_TargetOS_FreeRTOS ON )
else()
    message( ERROR "[RGH] Target operating system \"${RGH_TARGET_OS}\" is not valid." )
    return()
endif()


include( FetchContent )

if( POLICY CMP0169 )
    cmake_policy( SET CMP0169 OLD )
endif()

set( RGH_Flag_ExcomActive OFF )
function( RGH_Fnc_PopExcom excom_name )
    if( NOT DEFINED RGH_EXCOM_POPCOUNT )
        message( ERROR "[RGH] Excom popcount is not defined." )
        set( RGH_Flag_ExcomActive OFF PARENT_SCOPE )
        return()
    endif()

    string( SUBSTRING "${RGH_EXCOM_POPCOUNT}" 0 1 _popped )
    string( SUBSTRING "${RGH_EXCOM_POPCOUNT}" 1 -1 _next_popcount )

    set( RGH_EXCOM_POPCOUNT ${_next_popcount} PARENT_SCOPE )
    
    if( NOT _popped STREQUAL "y" )
        message( STATUS "[RGH] Bypassing excom \"${excom_name}\". Remaining: ${_next_popcount}." )
        set( RGH_Flag_ExcomActive OFF PARENT_SCOPE )
    else()
        message( STATUS "[RGH] Diving excom \"${excom_name}\". Remaining: ${_next_popcount}." )
        set( RGH_Flag_ExcomActive ON PARENT_SCOPE )
    endif()
endfunction()

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


include( CheckIPOSupported )
check_ipo_supported( RESULT _rgh_ipo_supported OUTPUT _ )
if( _rgh_ipo_supported )
    set( CMAKE_INTERPROEDURAL_OPTIMIZATION ON )
    message( STATUS "[RGH] IPO enabled." )
else()
    message( STATUS "[RGH] IPO not enabled." )
endif()

set( FETCHCONTENT_UPDATES_DISCONNECTED ON )


message( STATUS "[RGH] Diving BR plate." )
add_subdirectory( "brp" )
message( STATUS "[RGH] BR plate done." )

message( STATUS "[RGH] Diving GE plate." )
add_subdirectory( "gep" )
message( STATUS "[RGH] GE plate done.." )

if( RGH_Opt_TargetPlate_uC )
    message( STATUS "[RGH] Diving uC plate." )
    add_subdirectory( "ucp" )
    message( STATUS "[RGH] uC plate done." )
endif()

if( RGH_Opt_TargetPlate_OS )
    message( STATUS "[RGH] Diving OS plate." )
    add_subdirectory( "osp" )
    message( STATUS "[RGH] OS plate done." )
endif()

foreach( rupture ${RGH_MAKE_RUPTURES} )
    add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/ruptures/${rupture}" )
endforeach()

message( STATUS "[RGH] CMake done." )
