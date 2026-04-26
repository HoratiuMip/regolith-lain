cmake_minimum_required( VERSION 3.20.0 )

set( _excom_dir "${CMAKE_CURRENT_LIST_DIR}/excom" )

file( GLOB _source_files "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" )

add_library( RGH_GEp STATIC ${_source_files} )

target_link_libraries( RGH_GEp PUBLIC RGH_BRp )
target_include_directories( RGH_GEp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include" )
