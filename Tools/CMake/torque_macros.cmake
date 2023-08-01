# -----------------------------------------------------------------------------
# Copyright (c) 2014 GarageGames, LLC
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
# -----------------------------------------------------------------------------
################# Helper Functions ###################
macro(setupVersionNumbers)
    set(TORQUE_APP_VERSION_MAJOR 1 CACHE STRING "")
    set(TORQUE_APP_VERSION_MINOR 0 CACHE STRING "")
    set(TORQUE_APP_VERSION_PATCH 0 CACHE STRING "")
    set(TORQUE_APP_VERSION_TWEAK 0 CACHE STRING "")

    mark_as_advanced(TORQUE_APP_VERSION_TWEAK)
    MATH(EXPR TORQUE_APP_VERSION "${TORQUE_APP_VERSION_MAJOR} * 1000 + ${TORQUE_APP_VERSION_MINOR} * 100 + ${TORQUE_APP_VERSION_PATCH} * 10 + ${TORQUE_APP_VERSION_TWEAK}")
    set(TORQUE_APP_VERSION_STRING "${TORQUE_APP_VERSION_MAJOR}.${TORQUE_APP_VERSION_MINOR}.${TORQUE_APP_VERSION_PATCH}.${TORQUE_APP_VERSION_TWEAK}")
endmacro()

function(installTemplate templateName)
  message("Prepare Template(${templateName}) install...")

  add_subdirectory("${CMAKE_SOURCE_DIR}/Templates/${templateName}")
endfunction()

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

# Helper function to add a directory to the TORQUE_SOURCE_FILES variable. It automatically searches for .cpp and .h files in the
# specified directory then adds them to the TORQUE_SOURCE_FILES variable.
macro (torqueAddSourceDirectories)
  foreach(ARGUMENT ${ARGV})
    file(GLOB SCANNED_SOURCE_FILES "${ARGUMENT}/*.cpp")
    file(GLOB SCANNED_INCLUDE_FILES "${ARGUMENT}/*.h")

    if (APPLE)
      file(GLOB SCANNED_MAC_FILES "${ARGUMENT}/*.mm")
    endif (APPLE)

    # Set in both current and parent scope so this macro can be used from loaded modules
    set(TORQUE_SOURCE_FILES ${TORQUE_SOURCE_FILES} ${SCANNED_SOURCE_FILES} ${SCANNED_INCLUDE_FILES} ${SCANNED_MAC_FILES})
    set(TORQUE_SOURCE_FILES ${TORQUE_SOURCE_FILES} PARENT_SCOPE)
  endforeach()
endmacro (torqueAddSourceDirectories)

################# Set Conditional Engine Defines ###################
macro (forwardDef flag)
    if (${flag})
        set(TORQUE_COMPILE_DEFINITIONS ${TORQUE_COMPILE_DEFINITIONS} ${flag})
    endif()
endmacro(forwardDef)

macro (advanced_option flag description state)
    option(${flag} ${description} ${state})
    mark_as_advanced(${flag})
endmacro(advanced_option)
################# additional preprocessor defines ###################
macro(__addDef def config)
    # two possibilities: a) target already known, so add it directly, or b) target not yet known, so add it to its cache
    if(TARGET ${PROJECT_NAME})
        #message(STATUS "directly applying defs: ${PROJECT_NAME} with config ${config}: ${def}")
        if("${config}" STREQUAL "")
            set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS ${def})
        else()
            set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:${config}>:${def}>)
        endif()
    else()
        if("${config}" STREQUAL "")
            list(APPEND ${PROJECT_NAME}_defs_ ${def})
        else()
            list(APPEND ${PROJECT_NAME}_defs_ $<$<CONFIG:${config}>:${def}>)
        endif()
        #message(STATUS "added definition to cache: ${PROJECT_NAME}_defs_: ${${PROJECT_NAME}_defs_}")
    endif()
endmacro()

# adds a definition: argument 1: Nothing(for all), _DEBUG, _RELEASE, <more build configurations>
macro(addDef def)
    set(def_configs "")
    if(${ARGC} GREATER 1)
        foreach(config ${ARGN})
            __addDef(${def} ${config})
        endforeach()
    else()
        __addDef(${def} "")
    endif()
endmacro()

# this applies cached definitions onto the target must come *after* target_compile_definitions
macro(append_defs)
    if(DEFINED ${PROJECT_NAME}_defs_)
        set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS ${${PROJECT_NAME}_defs_})
        #message(STATUS "applying defs to project ${PROJECT_NAME}: ${${PROJECT_NAME}_defs_}")
    else()
        #message(STATUS "NO ${PROJECT_NAME}_defs_ defined!")
    endif()    
endmacro()

################# file filtering ###################
macro (filterOut)
  foreach(ARGUMENT ${ARGV})
    list(REMOVE_ITEM TORQUE_SOURCE_FILES "${CMAKE_SOURCE_DIR}/Engine/source/${ARGUMENT}")
  endforeach()
endmacro (filterOut)

################# apple macros ###################
# find_dependencies: Check linked interface and direct dependencies of target
function(find_dependencies)
  set(oneValueArgs TARGET FOUND_VAR)
  set(multiValueArgs)
  cmake_parse_arguments(var "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED is_root)
    # Root of recursive dependency resolution
    set(is_root TRUE)
    set(nested_depth 0)
  else()
    # Branch of recursive dependency resolution
    set(is_root FALSE)
    math(EXPR nested_depth "${nested_depth}+1")
  endif()

  # * LINK_LIBRARIES are direct dependencies
  # * INTERFACE_LINK_LIBRARIES are transitive dependencies
  get_target_property(linked_libraries ${var_TARGET} LINK_LIBRARIES)
  get_target_property(interface_libraries ${var_TARGET} INTERFACE_LINK_LIBRARIES)
  message(DEBUG "[${nested_depth}] Linked libraries in target ${var_TARGET}: ${linked_libraries}")
  message(DEBUG "[${nested_depth}] Linked interface libraries in target ${var_TARGET}: ${interface_libraries}")

  # Consider CMake targets only
  list(FILTER linked_libraries INCLUDE REGEX ".+::.+")
  list(FILTER interface_libraries INCLUDE REGEX ".+::.+")

  foreach(library IN LISTS linked_libraries interface_libraries)
    if(NOT library)
      continue()
    elseif(library MATCHES "\\$<.*:[^>]+>")
      # Generator expression found
      if(library MATCHES "\\$<\\$<PLATFORM_ID:[^>]+>:.+>")
        # Platform-dependent generator expression found - platforms are a comma-separated list of CMake host OS
        # identifiers. Convert to CMake list and check if current host os is contained in list.
        string(REGEX REPLACE "\\$<\\$<PLATFORM_ID:([^>]+)>:([^>]+)>" "\\1;\\2" gen_expression "${library}")
        list(GET gen_expression 0 gen_platform)
        list(GET gen_expression 1 gen_library)
        string(REPLACE "," ";" gen_platform "${gen_platform}")
        if(CMAKE_SYSTEM_NAME IN_LIST platform)
          set(library "${gen_library}")
        else()
          continue()
        endif()
      elseif(library MATCHES "\\$<\\$<BOOL:[^>]+>:.+>")
        # Boolean generator expression found - consider parameter a CMake variable that resolves into a CMake-like
        # boolean value for a simple conditional check.
        string(REGEX REPLACE "\\$<\\$<BOOL:([^>]+)>:([^>]+)>" "\\1;\\2" gen_expression "${library}")
        list(GET gen_expression 0 gen_boolean)
        list(GET gen_expression 1 gen_library)
        if(${gen_boolean})
          set(library "${gen_library}")
        else()
          continue()
        endif()
      elseif(library MATCHES "\\$<TARGET_NAME_IF_EXISTS:[^>]+>")
        # Target-dependent generator expression found - consider parameter to be a CMake target identifier and check for
        # target existence.
        string(REGEX REPLACE "\\$<TARGET_NAME_IF_EXISTS:([^>]+)>" "\\1" gen_target "${library}")
        if(TARGET ${gen_target})
          set(library "${gen_target}")
        else()
          continue()
        endif()
      elseif(library MATCHES "\\$<.*Qt6::EntryPointPrivate>" OR library MATCHES "\\$<.*Qt6::QDarwin.+PermissionPlugin>")
        # Known Qt6-specific generator expression, ignored.
        continue()
      else()
        # Unknown or unimplemented generator expression found - abort script run to either add to ignore list or
        # implement detection.
        message(FATAL_ERROR "${library} is an unsupported generator expression for linked libraries.")
      endif()
    endif()

    message(DEBUG "[${nested_depth}] Found ${library}...")

    if(NOT library IN_LIST ${var_FOUND_VAR})
      list(APPEND found_libraries ${library})
      # Enter recursive branch
      find_dependencies(TARGET ${library} FOUND_VAR ${var_FOUND_VAR})
    endif()
  endforeach()

  if(NOT is_root)
    set(found_libraries
        ${found_libraries}
        PARENT_SCOPE)
    # Exit recursive branch
    return()
  endif()

  list(REMOVE_DUPLICATES found_libraries)
  list(APPEND ${var_FOUND_VAR} ${found_libraries})
  set(${var_FOUND_VAR}
      ${${var_FOUND_VAR}}
      PARENT_SCOPE)
endfunction()

macro(addFramework framework)
	if (APPLE)
		set(TORQUE_LINK_FRAMEWORKS ${TORQUE_LINK_FRAMEWORKS} "$<LINK_LIBRARY:FRAMEWORK,${framework}.framework>")
	endif()
endmacro()

# _bundle_dependencies: Resolve 3rd party dependencies and add them to macOS app bundle
function(_bundle_dependencies target)
  message(DEBUG "Discover dependencies of target ${target}...")
  set(found_dependencies)
  find_dependencies(TARGET ${target} FOUND_VAR found_dependencies)

  list(REMOVE_DUPLICATES found_dependencies)

  set(library_paths)
  file(GLOB sdk_library_paths /Applications/Xcode*.app)
  set(system_library_path "/usr/lib/")

  foreach(library IN LISTS found_dependencies)
    get_target_property(library_type ${library} TYPE)
    get_target_property(is_framework ${library} FRAMEWORK)
    get_target_property(is_imported ${library} IMPORTED)

    if(is_imported)
      get_target_property(imported_location ${library} LOCATION)
      if(NOT imported_location)
        continue()
      endif()

      set(is_xcode_framework FALSE)
      set(is_system_framework FALSE)

      foreach(sdk_library_path IN LISTS sdk_library_paths)
        if(is_xcode_framework)
          break()
        endif()
        cmake_path(IS_PREFIX sdk_library_path "${imported_location}" is_xcode_framework)
      endforeach()
      cmake_path(IS_PREFIX system_library_path "${imported_location}" is_system_framework)

      if(is_system_framework OR is_xcode_framework)
        continue()
      elseif(is_framework)
        file(REAL_PATH "../../.." library_location BASE_DIRECTORY "${imported_location}")
      elseif(NOT library_type STREQUAL "STATIC_LIBRARY")
        if(NOT imported_location MATCHES ".+\\.a")
          set(library_location "${imported_location}")
        else()
          continue()
        endif()
      else()
        continue()
      endif()

      list(APPEND library_paths ${library_location})
    elseif(NOT is_imported AND library_type STREQUAL "SHARED_LIBRARY")
      list(APPEND library_paths ${library})
    endif()
  endforeach()

  list(REMOVE_DUPLICATES library_paths)
  set_property(
    TARGET ${target}
    APPEND
    PROPERTY XCODE_EMBED_FRAMEWORKS ${library_paths})
endfunction()