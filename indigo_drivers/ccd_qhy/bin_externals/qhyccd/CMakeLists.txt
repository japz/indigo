cmake_minimum_required(VERSION 2.8)
PROJECT(libqhy C CXX)

set(FIRMWARE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/firmware/qhy/")
set(UDEVRULES_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")

IF(UNIX AND NOT WIN32)
    IF(NOT SKIP_LIBS_INSTALL MATCHES "True")
        IF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")
            exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/lib/Linux/arm/libqhy.a ${CMAKE_BINARY_DIR}/libqhy.a)
        ELSEIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv7l")
            exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/lib/Linux/arm/libqhy.a ${CMAKE_BINARY_DIR}/libqhy.a)
        ELSEIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
            exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/lib/Linux/arm64/libqhy.a ${CMAKE_BINARY_DIR}/libqhy.a)
        ELSEIF (APPLE)
            exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/lib/macOS/libqhy.a ${CMAKE_BINARY_DIR}/libqhy.a)
        ELSEIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
            exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/lib/Linux/x64/libqhy.a ${CMAKE_BINARY_DIR}/libqhy.a)
        ELSE()
            exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/lib/Linux/x86/libqhy.a ${CMAKE_BINARY_DIR}/libqhy.a)
        ENDIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")

        install( FILES ${CMAKE_BINARY_DIR}/libqhy.a DESTINATION ${LIB_INSTALL_DIR}${LIB_POSTFIX})
        file(GLOB QHY_HEADERS "include/*.h")
        install( FILES ${QHY_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR})
    ENDIF(NOT SKIP_LIBS_INSTALL MATCHES "True")

    IF(NOT SKIP_FIRMWARE_INSTALL MATCHES "True")
        file(GLOB QHY_HEX_FIRMWARE "firmware/*.HEX")
        file(GLOB QHY_IMG_FIRMWARE "firmware/*.img")
        install( FILES ${QHY_HEX_FIRMWARE} ${QHY_IMG_FIRMWARE} DESTINATION ${FIRMWARE_INSTALL_DIR})
        install( FILES 85-qhyccd.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
    ENDIF(NOT SKIP_FIRMWARE_INSTALL MATCHES "True")
ENDIF(UNIX AND NOT WIN32)
