# Navigation module
option(TORQUE_NAVIGATION "Enable Navigation module" ON)

if(TORQUE_NAVIGATION)
  message("Enabling Navigation Module")
  set(NAVIGATION_DIR "${CMAKE_SOURCE_DIR}/Engine/source/navigation")

  moduleAddSourceDirectories(TORQUE_NAV_SOURCES "${NAVIGATION_DIR}")
  
  if(TORQUE_TOOLS)
    moduleAddSourceDirectories(TORQUE_NAV_SOURCES "${NAVIGATION_DIR}/gui" "${NAVIGATION_DIR}/gui/tools")
    set(TORQUE_LINK_LIBRARIES ${TORQUE_LINK_LIBRARIES} DebugUtils)
  endif(TORQUE_TOOLS)
  
  set(TORQUE_SOURCE_FILES ${TORQUE_SOURCE_FILES} ${TORQUE_NAV_SOURCES})
  set(TORQUE_LINK_LIBRARIES ${TORQUE_LINK_LIBRARIES} Detour DetourCrowd Recast DetourTileCache)
  set(TORQUE_COMPILE_DEFINITIONS ${TORQUE_COMPILE_DEFINITIONS} recast TORQUE_NAVIGATION_ENABLED)

  # The build demo doesnt seem to work so keep it off regardless.
  advanced_option(RECASTNAVIGATION_TESTS "Build tests" OFF)
  advanced_option(RECASTNAVIGATION_EXAMPLES "Build examples" OFF)
  advanced_option(RECASTNAVIGATION_DEMO "Build demo" OFF)
  add_subdirectory("${TORQUE_LIB_ROOT_DIRECTORY}/recast" ${TORQUE_LIB_TARG_DIRECTORY}/recast EXCLUDE_FROM_ALL)
  source_group(TREE "${CMAKE_SOURCE_DIR}/Engine/source/navigation/" PREFIX "Modules/NAVIGATION" FILES ${TORQUE_NAV_SOURCES})
endif(TORQUE_NAVIGATION)
