include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_macros.cmake")

if(IOS)
    include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_ios_configs.cmake")
else()
    include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_configs.cmake")
endif()
cmake_minimum_required (VERSION 3.21.0)
set (CMAKE_CXX_STANDARD 17)

# Ensure multi-core compilation is enabled for everything
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

project(${TORQUE_APP_NAME})

if(APPLE)
   enable_language(OBJC)
   set(TORQUE_APPLE_TARGET "macOS" CACHE STRING "The apple target")
    set_property(CACHE TORQUE_APPLE_TARGET PROPERTY STRINGS macOS iphoneOS)

    set(TORQUE_IOS_PLATFORM "OS" CACHE STRING "The ios platform")
    set_property(CACHE TORQUE_IOS_PLATFORM PROPERTY STRINGS OS SIMULATOR)
    
    if(TORQUE_APPLE_TARGET MATCHES "iphoneOS")
        set(IOS TRUE)
    endif()

    if(APPLE AND NOT IOS)
        option(TORQUE_MACOS_UNIVERSAL_BINARY OFF)
    
        if(TORQUE_MACOS_UNIVERSAL_BINARY)
            set(ARCHITECTURE_STRING_APPLE "x86_64;arm64")
            set(DEPLOYMENT_TARGET_APPLE "10.14")
        else()
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
                set(ARCHITECTURE_STRING_APPLE "arm64")
                set(DEPLOYMENT_TARGET_APPLE "11.0")
            else()
                set(ARCHITECTURE_STRING_APPLE "x86_64")
                set(DEPLOYMENT_TARGET_APPLE "10.14")
            endif()
        endif()
    
        set(CMAKE_OSX_ARCHITECTURES ${ARCHITECTURE_STRING_APPLE} CACHE STRING "OSX Architecture" FORCE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET_APPLE} CACHE STRING "OSX Deployment target" FORCE)
  endif()

endif(APPLE)

if(IOS)
    
    set (CMAKE_SYSTEM_NAME iOS)

    # Check the platform selection and setup for developer root
    if (${TORQUE_IOS_PLATFORM} STREQUAL "OS")
        set (IOS_PLATFORM_LOCATION "iPhoneOS.platform")
    elseif (${TORQUE_IOS_PLATFORM} STREQUAL "SIMULATOR")
        set (IOS_PLATFORM_LOCATION "iPhoneSimulator.platform")
    endif ()
    
    # Setup iOS developer location
    if (NOT DEFINED CMAKE_IOS_DEVELOPER_ROOT)
        set (CMAKE_IOS_DEVELOPER_ROOT "/Applications/Xcode.app/Contents/Developer/Platforms/${IOS_PLATFORM_LOCATION}/Developer")
    endif (NOT DEFINED CMAKE_IOS_DEVELOPER_ROOT)

    set (CMAKE_IOS_DEVELOPER_ROOT ${CMAKE_IOS_DEVELOPER_ROOT} CACHE PATH "Location of iOS Platform")

    # Find and use the most recent iOS sdk 
    if (NOT DEFINED CMAKE_IOS_SDK_ROOT)
        file (GLOB _CMAKE_IOS_SDKS "${CMAKE_IOS_DEVELOPER_ROOT}/SDKs/*")
        if (_CMAKE_IOS_SDKS) 
            list (SORT _CMAKE_IOS_SDKS)
            list (REVERSE _CMAKE_IOS_SDKS)
            list (GET _CMAKE_IOS_SDKS 0 CMAKE_IOS_SDK_ROOT)
        else (_CMAKE_IOS_SDKS)
            message (FATAL_ERROR "No iOS SDK's found in default seach path ${CMAKE_IOS_DEVELOPER_ROOT}. Manually set CMAKE_IOS_SDK_ROOT or install the iOS SDK.")
        endif (_CMAKE_IOS_SDKS)
        message (STATUS "Toolchain using default iOS SDK: ${CMAKE_IOS_SDK_ROOT}")
    endif (NOT DEFINED CMAKE_IOS_SDK_ROOT)
    set (CMAKE_IOS_SDK_ROOT ${CMAKE_IOS_SDK_ROOT} CACHE PATH "Location of the selected iOS SDK")

    # Set the sysroot default to the most recent SDK
    set (CMAKE_OSX_SYSROOT ${CMAKE_IOS_SDK_ROOT} CACHE PATH "Sysroot used for iOS support")

    # set the architecture for iOS - using ARCHS_STANDARD_32_BIT sets armv6,armv7 and appears to be XCode's standard. 
    # The other value that works is ARCHS_UNIVERSAL_IPHONE_OS but that sets armv7 only
    set (CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_32_BIT)" CACHE STRING  "Build architecture for iOS")

    # Set the find root to the iOS developer roots and to user defined paths
    set (CMAKE_FIND_ROOT_PATH ${CMAKE_IOS_DEVELOPER_ROOT} ${CMAKE_IOS_SDK_ROOT} ${CMAKE_PREFIX_PATH} CACHE STRING  "iOS find search path root")

    # default to searching for frameworks first
    set (CMAKE_FIND_FRAMEWORK FIRST)

    # set up the default search directories for frameworks
    set (CMAKE_SYSTEM_FRAMEWORK_PATH
        ${CMAKE_IOS_SDK_ROOT}/System/Library/Frameworks
        ${CMAKE_IOS_SDK_ROOT}/System/Library/PrivateFrameworks
        ${CMAKE_IOS_SDK_ROOT}/Developer/Library/Frameworks
    )

    # only search the iOS sdks, not the remainder of the host filesystem
    set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
    set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

# An application name must be set first
set(TORQUE_APP_NAME "" CACHE STRING "the app name")
if("${TORQUE_APP_NAME}" STREQUAL "")
	message(FATAL_ERROR "Please set TORQUE_APP_NAME first")
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
