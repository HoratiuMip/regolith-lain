cmake_minimum_required( VERSION 3.20.0 )

set( _excom_dir "${CMAKE_CURRENT_LIST_DIR}/excom" )

configure_file( "${CMAKE_CURRENT_LIST_DIR}/include/rgh/brp/descriptor.hpp.in" "${CMAKE_CURRENT_LIST_DIR}/include/rgh/brp/descriptor.hpp" )

file( GLOB _source_files "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" )

add_library( RGH_BRp STATIC ${_source_files} )

target_include_directories( RGH_BRp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include" )
