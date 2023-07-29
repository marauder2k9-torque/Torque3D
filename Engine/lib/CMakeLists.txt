if(NOT IOS)
    include(torqueLibs.cmake)
endif()

if(IOS)
    include(torqueIOSLibs.cmake)
endif()