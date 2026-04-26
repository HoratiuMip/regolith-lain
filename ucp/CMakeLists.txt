cmake_minimum_required( VERSION 3.20.0 )

file( GLOB _source_files "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" )

add_library( RGH_uCp STATIC ${_source_files} )

target_link_libraries( RGH_uCp PUBLIC RGH_GEp )
target_include_directories( RGH_uCp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include" )

if( RGH_Opt_TargetOS_FreeRTOS )
    target_compile_definitions( RGH_uCp PUBLIC RGH_TARGET_OS_FREERTOS )
endif()