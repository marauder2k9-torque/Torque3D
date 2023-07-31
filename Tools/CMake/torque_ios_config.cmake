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
set(TORQUE_LINK_LIBRARIES tinyxml collada ljpeg squish png_static opcode glad pcre zlib)

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

if(APPLE)
   set(TORQUE_APPLE_TARGET "" CACHE STRING "The apple target")
    set_property(CACHE TORQUE_APPLE_TARGET PROPERTY STRINGS macOS iphoneOS)

    set(TORQUE_IOS_PLATFORM "OS" CACHE STRING "The ios platform")
    set_property(CACHE TORQUE_IOS_PLATFORM PROPERTY STRINGS OS SIMULATOR)

    if("${TORQUE_APPLE_TARGET}" STREQUAL "")
      message(FATAL_ERROR "Please set TORQUE_APPLE_TARGET first")
    endif()
      
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
  set(CMAKE_SYSTEM_NAME "iOS")
  # In order to build for both device and simulator, this needs to be empty,
  # or the CMAKE_OSX_DEPLOYMENT_TARGET setting below is not handled correctly.
  # Note, this seems to cause an issue with repeating builds on some cmake versions,
  # the long term fix will likely be to not handle both device and simulator with
  # the same toolchain, and instead merge the libraries after the fact.
  set(CMAKE_OSX_SYSROOT "")
  set(CMAKE_OSX_ARCHITECTURES "arm64;armv7;x86_64;i386" CACHE STRING "")
  set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
  set(IOS_PLATFORM_LOCATION "iPhoneOS.platform;iPhoneSimulator.platform")

  set(CMAKE_IOS_INSTALL_UNIVERSAL_LIBS "YES")
  set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO")
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
  set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "YES")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "8.0" CACHE STRING "")

  set(IOS ON CACHE BOOL "")
endif()

