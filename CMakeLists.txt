include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_macros.cmake")
include("${CMAKE_SOURCE_DIR}/Tools/CMake/torque_configs.cmake")

# An application name must be set first
set(TORQUE_APP_NAME "" CACHE STRING "the app name")
if("${TORQUE_APP_NAME}" STREQUAL "")
	message(FATAL_ERROR "Please set TORQUE_APP_NAME first")
endif()

# in apple we need to select the target and ios platform.
if(APPLE)
    set(TORQUE_APPLE_TARGET "" CACHE STRING "The apple target")
    set_property(CACHE TORQUE_APPLE_TARGET PROPERTY STRINGS macOS iphoneOS)

    set(TORQUE_IOS_PLATFORM "" CACHE STRING "The ios platform")
    set_property(CACHE TORQUE_IOS_PLATFORM PROPERTY STRINGS OS SIMULATOR)

    if("${TORQUE_APPLE_TARGET}" STREQUAL "")
	    message(FATAL_ERROR "On Apple set TORQUE_APPLE_TARGET")
    endif()

    if(TORQUE_APPLE_TARGET MATCHES "iphoneOS")
        set(IOS TRUE)
    endif()
endif(APPLE)

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
endif(APPLE AND NOT IOS)

if(IOS)
    if("${TORQUE_IOS_PLATFORM}" STREQUAL "")
        message(FATAL_ERROR "Please choose set TORQUE_IOS_PLATFORM")
    endif()

    if (TORQUE_IOS_PLATFORM STREQUAL "OS")
        set(XCODE_IOS_PLATFORM iphoneos)
    elseif(TORQUE_IOS_PLATFORM STREQUAL "SIMULATOR")
        set(XCODE_IOS_PLATFORM iphonesimulator)
    else()
        message(FATAL_ERROR "Invalid IOS_PLATFORM: ${IOS_PLATFORM}")
    endif()

    if (NOT CMAKE_OSX_SYSROOT)
        execute_process(COMMAND xcodebuild -version -sdk ${XCODE_IOS_PLATFORM} Path
            OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        message(STATUS "Using SDK: ${CMAKE_OSX_SYSROOT} for platform: ${IOS_PLATFORM}")
    endif()
    if (NOT EXISTS ${CMAKE_OSX_SYSROOT})
        message(FATAL_ERROR "Invalid CMAKE_OSX_SYSROOT: ${CMAKE_OSX_SYSROOT} "
            "does not exist.")
    endif()

    if (NOT CMAKE_IOS_DEVELOPER_ROOT)
        get_filename_component(IOS_PLATFORM_SDK_DIR ${CMAKE_OSX_SYSROOT} PATH)
        get_filename_component(CMAKE_IOS_DEVELOPER_ROOT ${IOS_PLATFORM_SDK_DIR} PATH)
    endif()
        if (NOT EXISTS ${CMAKE_IOS_DEVELOPER_ROOT})
        message(FATAL_ERROR "Invalid CMAKE_IOS_DEVELOPER_ROOT: "
            "${CMAKE_IOS_DEVELOPER_ROOT} does not exist.")
    endif()

    execute_process(COMMAND xcodebuild -sdk ${CMAKE_OSX_SYSROOT} -version SDKVersion
                    OUTPUT_VARIABLE IOS_SDK_VERSION
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(CMAKE_SYSTEM_NAME iOS)
    set(CMAKE_SYSTEM_VERSION ${IOS_SDK_VERSION})
    set(CMAKE_OSX_ARCHITECTURES $(ARCH_STANDARD))
    set(CMAKE_IOS_INSTALL_COMBINED YES)

    if (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)
        find_program(CMAKE_INSTALL_NAME_TOOL install_name_tool)
    endif (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)
    # Set the find root to the iOS developer roots and to user defined paths.
    set(CMAKE_FIND_ROOT_PATH ${CMAKE_IOS_DEVELOPER_ROOT} ${CMAKE_OSX_SYSROOT}
    ${CMAKE_PREFIX_PATH} CACHE STRING  "iOS find search path root" FORCE)
    # Default to searching for frameworks first.
    set(CMAKE_FIND_FRAMEWORK FIRST)
    # Set up the default search directories for frameworks.
    set(CMAKE_SYSTEM_FRAMEWORK_PATH
    ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks
    ${CMAKE_OSX_SYSROOT}/System/Library/PrivateFrameworks
    ${CMAKE_OSX_SYSROOT}/Developer/Library/Frameworks)
    # Only search the specified iOS SDK, not the remainder of the host filesystem.
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

    set(CMAKE_SHARED_LINKER_FLAGS "-rpath @executable_path/Frameworks -rpath @loader_path/Frameworks")
    set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-dynamiclib -Wl,-headerpad_max_install_names")
    set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle -Wl,-headerpad_max_install_names")
    set(CMAKE_SHARED_MODULE_LOADER_C_FLAG "-Wl,-bundle_loader,")
    set(CMAKE_SHARED_MODULE_LOADER_CXX_FLAG "-Wl,-bundle_loader,")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".tbd" ".dylib" ".so" ".a")
    set(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-install_name")

    include("${CMAKE_SOURCE_DIR}/Tools/CMake/setup/torque-ios-configs.cmake")
endif(IOS)

cmake_minimum_required (VERSION 3.21.0)
set (CMAKE_CXX_STANDARD 17)

# Ensure multi-core compilation is enabled for everything
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

project(${TORQUE_APP_NAME})

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
