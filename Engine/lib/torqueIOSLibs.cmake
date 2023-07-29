# Ask CMake to perform builds in a temporary directory for all of these.
# We also use EXCLUDE_FROM_ALL to ensure we only build and install what we want
#ZLIB
get_filename_component(ZLIB_ROOT "zlib" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ZLIB_ROOT "${ZLIB_ROOT}" CACHE STRING "ZLib root location" FORCE)
mark_as_advanced(ZLIB_ROOT)
# Png depends on zlib
add_subdirectory(zlib ${CMAKE_BINARY_DIR}/temp/zlib EXCLUDE_FROM_ALL)

if (TORQUE_CPU_ARM32 OR TORQUE_CPU_ARM64)
	set(PNG_ARM_NEON on CACHE BOOL "" FORCE)
endif (TORQUE_CPU_ARM32 OR TORQUE_CPU_ARM64)

#PNG
set(PNG_STATIC on CACHE BOOL "" FORCE)
mark_as_advanced(PNG_STATIC)
set(PNG_SHARED off CACHE BOOL "" FORCE)
mark_as_advanced(PNG_SHARED)
set(PNG_BUILD_ZLIB on CACHE BOOL "" FORCE)
mark_as_advanced(PNG_BUILD_ZLIB)
set(PNG_TESTS off CACHE BOOL "" FORCE)
mark_as_advanced(PNG_TESTS)
set(PNG_HARDWARE_OPTIMIZATIONS on CACHE BOOL "" FORCE)
mark_as_advanced(PNG_HARDWARE_OPTIMIZATIONS)
if(APPLE)
    set(PNG_FRAMEWORK on CACHE BOOL "" FORCE)
    addDef(PNG_DEBUG Debug)
endif()
mark_as_advanced(PNG_DEBUG)
mark_as_advanced(PNG_FRAMEWORK)
mark_as_advanced(PNG_PREFIX)
add_subdirectory(lpng ${CMAKE_BINARY_DIR}/temp/lpng EXCLUDE_FROM_ALL)


add_subdirectory(ljpeg ${CMAKE_BINARY_DIR}/temp/ljpeg EXCLUDE_FROM_ALL)
add_subdirectory(tinyxml ${CMAKE_BINARY_DIR}/temp/tinyxml EXCLUDE_FROM_ALL)
add_subdirectory(opcode ${CMAKE_BINARY_DIR}/temp/opcode EXCLUDE_FROM_ALL)
add_subdirectory(pcre ${CMAKE_BINARY_DIR}/temp/pcre EXCLUDE_FROM_ALL)
add_subdirectory(squish ${CMAKE_BINARY_DIR}/temp/squish EXCLUDE_FROM_ALL)
add_subdirectory(convexDecomp ${CMAKE_BINARY_DIR}/temp/convexDecomp EXCLUDE_FROM_ALL)
add_subdirectory(collada ${CMAKE_BINARY_DIR}/temp/collada EXCLUDE_FROM_ALL)
add_subdirectory(glad ${CMAKE_BINARY_DIR}/temp/glad EXCLUDE_FROM_ALL)

#endif()