project(${TORQUE_APP_NAME})

if(WIN32)
  include(torqueWin.cmake)
endif(WIN32)

if(UNIX AND NOT APPLE)
  include(torqueLinux.cmake)
endif(UNIX AND NOT APPLE)

if(APPLE AND NOT IOS)
  include(torqueMac.cmake)
endif()

if(IOS)
  include(torqueIOS.cmake)
endif()
