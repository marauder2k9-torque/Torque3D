project(${TORQUE_APP_NAME})

if(WIN32)
  include(torqueDefault.cmake)
endif(WIN32)

if(UNIX AND NOT APPLE)
  include(torqueDefault.cmake)
endif(UNIX AND NOT APPLE)

if(APPLE AND NOT IOS)
  include(torqueDefault.cmake)
endif()

if(IOS)
  include(torqueIOS.cmake)
endif()