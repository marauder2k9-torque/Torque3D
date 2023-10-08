# Libsndfile module
option(TORQUE_SFX_LIBSND "Use LibSndfile loading" ON)

include(ExternalProject)

if(TORQUE_SFX_LIBSND)

set(THIRD_PARTY_DIR ${CMAKE_BINARY_DIR}/temp)

# ExternalProject_Add(
#     flac
#     GIT_REPOSITORY https://github.com/xiph/flac.git
#     GIT_TAG 1.4.3
#     PREFIX ${THIRD_PARTY_DIR}/flac
#     SOURCE_DIR ${THIRD_PARTY_DIR}/flac
#     STAMP_DIR ${THIRD_PARTY_DIR}/flac-stamp
#     UPDATE_COMMAND ""
#     PATCH_COMMAND ""
#     CMAKE_GENERATOR ${gen}
#     CMAKE_ARGS -DWITH_OGG:BOOL=OFF -DINSTALL_MANPAGES:BOOL=OFF -DBUILD_PROGRAMS:BOOL=OFF -DBUILD_EXAMPLES:BOOL=OFF -DBUILD_TESTING:BOOL=OFF -DBUILD_DOCS:BOOL=OFF
#     BUILD_IN_SOURCE 1
# )

set(BUILD_PROGRAMS OFF CACHE BOOL "Build and install programs" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build and install examples" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(BUILD_DOCS OFF CACHE BOOL "Build and install doxygen documents" FORCE)
set(WITH_FORTIFY_SOURCE OFF CACHE BOOL "Enable protection against buffer overflows" FORCE)
set(WITH_STACK_PROTECTOR OFF CACHE BOOL "Enable GNU GCC stack smash protection" FORCE)
set(INSTALL_MANPAGES OFF CACHE BOOL "Install MAN pages" FORCE)
set(INSTALL_PKGCONFIG_MODULES OFF CACHE BOOL "Install PkgConfig modules" FORCE)
set(INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "Install CMake package-config module" FORCE)
set(WITH_OGG OFF CACHE BOOL "ogg support (default: test for libogg)" FORCE)
set(WITH_AVX OFF CACHE BOOL "scxt is sse4.2 not ACXX" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/Engine/lib/libflac ${CMAKE_BINARY_DIR}/temp/flac EXCLUDE_FROM_ALL)
add_dependencies(FLAC libogg)

add_subdirectory(${CMAKE_SOURCE_DIR}/Engine/lib/libsndfile ${CMAKE_BINARY_DIR}/temp/libsndfile EXCLUDE_FROM_ALL)
add_dependencies(sndfile libogg FLAC libvorbis)

set(TORQUE_INCLUDE_DIRECTORIES ${TORQUE_INCLUDE_DIRECTORIES} "${CMAKE_SOURCE_DIR}/Engine/lib/libsndfile/include")
set(TORQUE_LINK_LIBRARIES ${TORQUE_LINK_LIBRARIES} sndfile)

endif(TORQUE_SFX_LIBSND)