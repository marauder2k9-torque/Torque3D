# Libsndfile module
option(TORQUE_SFX_LIBSND "Use LibSndfile loading" ON)

if(TORQUE_SFX_LIBSND)

set(THIRD_PARTY_DIR ${CMAKE_BINARY_DIR}/temp)
include(ExternalProject)

ExternalProject_Add(
    ogg
    GIT_REPOSITORY https://github.com/xiph/ogg.git
    GIT_TAG v1.3.5
    PREFIX ${THIRD_PARTY_DIR}
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_DIR}/ogg/install
)

set(ENV{Ogg_DIR} ${THIRD_PARTY_DIR}/ogg/install/lib/cmake/Ogg)
find_package(Ogg)

ExternalProject_Add(
    vorbis
    DEPENDS ogg
    GIT_REPOSITORY https://github.com/xiph/vorbis.git
    GIT_TAG v1.3.7
    PREFIX ${THIRD_PARTY_DIR}
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_DIR}/vorbis/install
    -DOGG_INCLUDE_DIR=${THIRD_PARTY_DIR}/ogg/install/include
    -DOGG_LIBRARY=${THIRD_PARTY_DIR}/ogg/install/lib/ogg.lib
)

ExternalProject_Add(
    opus
    DEPENDS ogg
    GIT_REPOSITORY https://github.com/xiph/opus.git
    GIT_TAG v1.4
    PREFIX ${THIRD_PARTY_DIR}
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_DIR}/opus/install
    -DOGG_INCLUDE_DIR=${THIRD_PARTY_DIR}/ogg/install/include
    -DOGG_LIBRARY=${THIRD_PARTY_DIR}/ogg/install/lib/ogg.lib
)


ExternalProject_Add(
    flac
    DEPENDS ogg
    GIT_REPOSITORY https://github.com/xiph/flac.git
    GIT_TAG 1.3.3
    PREFIX ${THIRD_PARTY_DIR}/flac
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_DIR}/flac/install
    -DOGG_INCLUDE_DIR=${THIRD_PARTY_DIR}/ogg/install/include
    -DOGG_LIBRARY=${THIRD_PARTY_DIR}/ogg/install/lib/ogg.lib
)

set(ENV{FLAC_DIR} ${THIRD_PARTY_DIR}/flac/install/share/FLAC/cmake)
set(ENV{Opus_DIR} ${THIRD_PARTY_DIR}/opus/install/lib/cmake/Opus)
set(ENV{Vorbis_DIR} ${THIRD_PARTY_DIR}/vorbis/install/lib/cmake/Vorbis)

find_package(FLAC)
find_package(Vorbis)
find_package(Opus)

add_subdirectory(${CMAKE_SOURCE_DIR}/Engine/lib/libsndfile ${CMAKE_BINARY_DIR}/temp/libsndfile EXCLUDE_FROM_ALL)
add_dependencies(sndfile ogg flac vorbis opus)

set(TORQUE_INCLUDE_DIRECTORIES ${TORQUE_INCLUDE_DIRECTORIES} ${CMAKE_SOURCE_DIR}/Engine/lib/libsndfile/include)

set(TORQUE_LINK_LIBRARIES ${TORQUE_LINK_LIBRARIES} sndfile)

endif(TORQUE_SFX_LIBSND)