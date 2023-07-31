include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_macros.cmake")

# include general options.
include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_options.cmake")

# any platform specific options go here.
if(APPLE)
    include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_apple_options.cmake")
endif()

include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_libconfig.cmake")

if(APPLE AND NOT IOS)
    include("${CMAKE_SOURCE_DIR}/Tools/CMake/MacOSX/torque_mac_config.cmake")
elseif(IOS)
    include("${CMAKE_SOURCE_DIR}/Tools/CMake/iOS/torque_ios_config.cmake")
endif()



cmake_minimum_required (VERSION 3.21.0)
set (CMAKE_CXX_STANDARD 17)

# Ensure multi-core compilation is enabled for everything
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

project(${TORQUE_APP_NAME})

if(APPLE)
    if(TORQUE_APPLE_TARGET MATCHES "iphoneOS")
        set(IOS TRUE)
    endif()
endif(APPLE)

if(IOS)
    #set (CMAKE_SYSTEM_NAME iOS)
endif()

# Once an app name is determined, we know what our project pathing structure should look like
set(TORQUE_APP_ROOT_DIRECTORY "${CMAKE_SOURCE_DIR}/My Projects/${TORQUE_APP_NAME}")
set(TORQUE_APP_GAME_DIRECTORY "${TORQUE_APP_ROOT_DIRECTORY}/game")

# Ensure all possible configurations end up in the project directory
set(CMAKE_INSTALL_PREFIX "${TORQUE_APP_ROOT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${TORQUE_APP_GAME_DIRECTORY}")
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${TORQUE_APP_GAME_DIRECTORY}" )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Detect CPU Information
if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    if( CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 8 )
        set( TORQUE_CPU_ARM64 ON )
    elseif( CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 4 )
        set( TORQUE_CPU_ARM32 ON )
    endif()
else()
    if( CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 8 )
        set( TORQUE_CPU_X64 ON )
    elseif( CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 4 )
        set( TORQUE_CPU_X32 ON )
    endif()
endif()

if(NOT TORQUE_SCRIPT_EXTENSION)
    set(TORQUE_SCRIPT_EXTENSION "tscript" CACHE STRING "The default script extension to use for TorqueScript files")
endif()
mark_as_advanced(TORQUE_SCRIPT_EXTENSION)

set(TORQUE_MODULE_USER_PATH "" CACHE PATH "Additional search path for modules aside from the default Tools/CMake/modules.")
mark_as_advanced(TORQUE_MODULE_USER_PATH)


# Install Torque template
if(NOT TORQUE_TEMPLATE)
    set(TORQUE_TEMPLATE "BaseGame" CACHE STRING "the template to use")
endif()

if (NOT TORQUE_INSTALLED_TEMPLATE)
    installTemplate(${TORQUE_TEMPLATE})
    set(TORQUE_INSTALLED_TEMPLATE TRUE CACHE BOOL "Whether or not the game template is installed. This may be reset to false (or removed) to force a reinstall.")
endif(NOT TORQUE_INSTALLED_TEMPLATE)

# Generate torqueConfig.h in our temp directory
configure_file("${CMAKE_SOURCE_DIR}/Tools/CMake/torqueConfig.h.in" "${CMAKE_BINARY_DIR}/temp/torqueConfig.h")

add_subdirectory(Engine)
