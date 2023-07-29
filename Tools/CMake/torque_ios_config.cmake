################# Initialize Common Variables ###################

# All include directories to search. Modules should append to this when they want includes to point
# into themselves.
set(TORQUE_INCLUDE_DIRECTORIES "")

# All library binaries to install. Modules should append to this the path of any library binaries (.so, .dylib, .dll)
# that should be installed next to the executable.
set(TORQUE_ADDITIONAL_LIBRARY_BINARIES "")

# All compile definitions. Modules should append to this if there is any special defines needed.
set(TORQUE_COMPILE_DEFINITIONS ICE_NO_DLL PCRE_STATIC TORQUE_ADVANCED_LIGHTING TORQUE_SHADERGEN
							   TORQUE_OPCODE TORQUE_COLLADA TORQUE_UNICODE UNICODE _UNICODE)

# All link libraries. Modules should append to this the path to specify additional link libraries (.a, .lib, .dylib, .so)
set(TORQUE_LINK_LIBRARIES tinyxml collada ljpeg squish png_static opcode glad pcre convexDecomp zlib)

#general
advanced_option(TORQUE_MULTITHREAD "Multi Threading" ON)
advanced_option(TORQUE_DISABLE_MEMORY_MANAGER "Disable memory manager" ON)

#fileIO
set(TORQUE_APP_PASSWORD "changeme" CACHE STRING "zip file password")
advanced_option(TORQUE_DISABLE_VIRTUAL_MOUNT_SYSTEM "Disable virtual mount system" OFF)
advanced_option(TORQUE_DISABLE_FIND_ROOT_WITHIN_ZIP "Disable reading root path from zip. Zips will be mounted in-place with file name as directory name." ON)
advanced_option(TORQUE_ZIP_DISK_LAYOUT "All zips must be placed in the executable directory and contain full paths to the files." OFF)
advanced_option(TORQUE_POSIX_PATH_CASE_INSENSITIVE "POSIX Pathing Case Insensitivity" ON)
advanced_option(TORQUE_ZIP_PATH_CASE_INSENSITIVE "ZIP Pathing Case Insensitivity" ON)
advanced_option(TORQUE_USE_ZENITY "use the Zenity backend for NFD" OFF)
advanced_option(TORQUE_SECURE_VFS "Secure VFS configuration. Arbitrary script access to file system will be heavily restricted." OFF)

#sfx
advanced_option(TORQUE_SFX_VORBIS "Vorbis Sound" OFF)
advanced_option(TORQUE_THEORA "Theora Video Support" OFF)
advanced_option(TORQUE_SFX_OPENAL "OpenAL Sound" OFF)

#gfx
advanced_option(TORQUE_DEBUG_GFX_MODE "triggers graphics debug mode" OFF)
advanced_option(TORQUE_ADVANCED_LIGHTING "Advanced Lighting" ON)
advanced_option(TORQUE_BASIC_LIGHTING "Basic Lighting" ON)
advanced_option(TORQUE_OPENGL "Allow OpenGL render" ON) # we need OpenGL to render on Linux/Mac

#mode
advanced_option(TORQUE_NO_DSO_GENERATION "skip storing compiled scripts" ON)
advanced_option(TORQUE_DYNAMIC_LIBRARY "Whether or not to build Torque as a dynamic library." OFF)
advanced_option(TORQUE_PLAYER "Playback only?" OFF)
advanced_option(TORQUE_DEBUG "T3D Debug mode" OFF)
#option(DEBUG_SPEW "more debug" OFF)
advanced_option(TORQUE_SHIPPING "T3D Shipping build?" OFF)
advanced_option(TORQUE_DEDICATED "Torque dedicated" OFF) # disables compiling in gfx and sfx frontend functionality

#tools
advanced_option(TORQUE_DEBUG_NET "debug network" OFF)
advanced_option(TORQUE_DEBUG_NET_MOVES "debug network moves" OFF)
advanced_option(TORQUE_ENABLE_ASSERTS "enables or disable asserts" OFF)
advanced_option(TORQUE_TOOLS "Enable or disable the tools" OFF)
advanced_option(TORQUE_ENABLE_PROFILER "Enable or disable the profiler" OFF)
advanced_option(TORQUE_SHOW_LEGACY_FILE_FIELDS "If on, shows legacy direct file path fields in the inspector." OFF)

#testing
advanced_option(TORQUE_TESTING "Unit test build" OFF)

setupVersionNumbers()

if(APPLE)
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

	# Force the compilers to gcc for iOS
	include (CMakeForceCompiler)
	CMAKE_FORCE_C_COMPILER (gcc gcc)
	CMAKE_FORCE_CXX_COMPILER (g++ g++)

	# Skip the platform compiler checks for cross compiling
	set (CMAKE_CXX_COMPILER_WORKS TRUE)
	set (CMAKE_C_COMPILER_WORKS TRUE)

	# All iOS/Darwin specific settings - some may be redundant
	set (CMAKE_SHARED_LIBRARY_PREFIX "lib")
	set (CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
	set (CMAKE_SHARED_MODULE_PREFIX "lib")
	set (CMAKE_SHARED_MODULE_SUFFIX ".so")
	set (CMAKE_MODULE_EXISTS 1)
	set (CMAKE_DL_LIBS "")

	set (CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG "-compatibility_version ")
	set (CMAKE_C_OSX_CURRENT_VERSION_FLAG "-current_version ")
	set (CMAKE_CXX_OSX_COMPATIBILITY_VERSION_FLAG "${CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG}")
	set (CMAKE_CXX_OSX_CURRENT_VERSION_FLAG "${CMAKE_C_OSX_CURRENT_VERSION_FLAG}")

	# Hidden visibilty is required for cxx on iOS 
	set (CMAKE_C_FLAGS "")
	set (CMAKE_CXX_FLAGS "-headerpad_max_install_names -fvisibility=hidden -fvisibility-inlines-hidden")

	set (CMAKE_C_LINK_FLAGS "-Wl,-search_paths_first ${CMAKE_C_LINK_FLAGS}")
	set (CMAKE_CXX_LINK_FLAGS "-Wl,-search_paths_first ${CMAKE_CXX_LINK_FLAGS}")

	set (CMAKE_PLATFORM_HAS_INSTALLNAME 1)
	set (CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-dynamiclib -headerpad_max_install_names")
	set (CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle -headerpad_max_install_names")
	set (CMAKE_SHARED_MODULE_LOADER_C_FLAG "-Wl,-bundle_loader,")
	set (CMAKE_SHARED_MODULE_LOADER_CXX_FLAG "-Wl,-bundle_loader,")
	set (CMAKE_FIND_LIBRARY_SUFFIXES ".dylib" ".so" ".a")

	# hardcode CMAKE_INSTALL_NAME_TOOL here to install_name_tool, so it behaves as it did before, Alex
	if (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)
		find_program(CMAKE_INSTALL_NAME_TOOL install_name_tool)
	endif (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)

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
	set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "OSX Deployment target" FORCE)
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

