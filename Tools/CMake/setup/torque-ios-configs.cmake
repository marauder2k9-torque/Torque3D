set(TORQUE_LINK_LIBRARIES tinyxml collada ljpeg squish png_static opcode glad pcre zlib)

advanced_set(TORQUE_DISABLE_MEMORY_MANAGER "Disable memory manager" ON)

#fileIO
advanced_set(TORQUE_DISABLE_VIRTUAL_MOUNT_SYSTEM "Disable virtual mount system" OFF)
advanced_set(TORQUE_DISABLE_FIND_ROOT_WITHIN_ZIP "Disable reading root path from zip. Zips will be mounted in-place with file name as directory name." ON)
advanced_set(TORQUE_ZIP_DISK_LAYOUT "All zips must be placed in the executable directory and contain full paths to the files." OFF)
advanced_set(TORQUE_POSIX_PATH_CASE_INSENSITIVE "POSIX Pathing Case Insensitivity" ON)
advanced_set(TORQUE_ZIP_PATH_CASE_INSENSITIVE "ZIP Pathing Case Insensitivity" ON)
advanced_set(TORQUE_USE_ZENITY "use the Zenity backend for NFD" OFF)
advanced_set(TORQUE_SECURE_VFS "Secure VFS configuration. Arbitrary script access to file system will be heavily restricted." OFF)

#sfx
advanced_set(TORQUE_SFX_VORBIS "Vorbis Sound" OFF)
advanced_set(TORQUE_THEORA "Theora Video Support" OFF)
advanced_set(TORQUE_SFX_OPENAL "OpenAL Sound" OFF)

#gfx
advanced_set(TORQUE_DEBUG_GFX_MODE "triggers graphics debug mode" OFF)
advanced_set(TORQUE_ADVANCED_LIGHTING "Advanced Lighting" ON)
advanced_set(TORQUE_BASIC_LIGHTING "Basic Lighting" ON)
advanced_set(TORQUE_OPENGL "Allow OpenGL render" ON) # we need OpenGL to render on Linux/Mac

#mode
advanced_set(TORQUE_NO_DSO_GENERATION "skip storing compiled scripts" ON)
advanced_set(TORQUE_DYNAMIC_LIBRARY "Whether or not to build Torque as a dynamic library." OFF)
advanced_set(TORQUE_PLAYER "Playback only?" OFF)
advanced_set(TORQUE_DEBUG "T3D Debug mode" OFF)
#option(DEBUG_SPEW "more debug" OFF)
advanced_set(TORQUE_SHIPPING "T3D Shipping build?" ON)
advanced_set(TORQUE_DEDICATED "Torque dedicated" OFF) # disables compiling in gfx and sfx frontend functionality

#tools
advanced_set(TORQUE_DEBUG_NET "debug network" OFF)
advanced_set(TORQUE_DEBUG_NET_MOVES "debug network moves" OFF)
advanced_set(TORQUE_ENABLE_ASSERTS "enables or disable asserts" OFF)
advanced_set(TORQUE_TOOLS "Enable or disable the tools" OFF)
advanced_set(TORQUE_ENABLE_PROFILER "Enable or disable the profiler" OFF)
advanced_set(TORQUE_SHOW_LEGACY_FILE_FIELDS "If on, shows legacy direct file path fields in the inspector." OFF)

#testing
advanced_set(TORQUE_TESTING "Unit test build" OFF)