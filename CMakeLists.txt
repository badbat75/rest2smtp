cmake_minimum_required(VERSION 3.10)

project(rest2smtp)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)

configure_file(src/config.h.in config.h @ONLY)
configure_file(rest2smtp.conf.in rest2smtp.conf @ONLY)

option(ENABLE_LTO "Enable Link Time Optimization" ON)
option(USE_SYSTEMD "Use systemd" ON)

add_executable(rest2smtp src/sw_config.c src/queue.c src/smtp_client.c src/api_server.c src/main.c)

pkg_check_modules(LIBCONFIG REQUIRED libconfig)
target_include_directories(rest2smtp PRIVATE ${LIBCONFIG_INCLUDE_DIRS})
target_link_directories(rest2smtp PRIVATE ${LIBCONFIG_LIBRARY_DIRS})
target_link_libraries(rest2smtp PRIVATE ${LIBCONFIG_LIBRARIES})

pkg_check_modules(LIBMICROHTTPD REQUIRED libmicrohttpd)
target_include_directories(rest2smtp PRIVATE ${LIBMICROHTTPD_INCLUDE_DIRS})
target_link_directories(rest2smtp PRIVATE ${LIBMICROHTTPD_LIBRARY_DIRS})
target_link_libraries(rest2smtp PRIVATE ${LIBMICROHTTPD_LIBRARIES})

pkg_check_modules(LIBJSONC REQUIRED json-c)
target_include_directories(rest2smtp PRIVATE ${LIBJSONC_INCLUDE_DIRS})
target_link_directories(rest2smtp PRIVATE  ${LIBJSONC_LIBRARY_DIRS})
target_link_libraries(rest2smtp PRIVATE ${LIBJSONC_LIBRARIES})

pkg_check_modules(LIBCURL REQUIRED libcurl)
target_include_directories(rest2smtp PRIVATE ${LIBCURL_INCLUDE_DIRS})
target_link_directories(rest2smtp PRIVATE ${LIBCURL_LIBRARY_DIRS})
target_link_libraries(rest2smtp PRIVATE ${LIBCURL_LIBRARIES})

pkg_check_modules(LIBCURL REQUIRED uuid)
target_include_directories(rest2smtp PRIVATE ${UUID_INCLUDE_DIRS})
target_link_directories(rest2smtp PRIVATE ${UUID_LIBRARY_DIRS})
target_link_libraries(rest2smtp PRIVATE "-luuid")

if (USE_SYSTEMD)
    pkg_check_modules(SYSTEMD QUIET libsystemd)
    if (SYSTEMD_FOUND)
        target_compile_definitions(rest2smtp PRIVATE SYSTEMD_FOUND=${SYSTEMD_FOUND})
        target_include_directories(rest2smtp PRIVATE ${SYSTEMD_INCLUDE_DIRS})
        target_link_directories(rest2smtp PRIVATE ${SYSTEMD_LIBRARY_DIRS})
        target_link_libraries(rest2smtp PRIVATE ${SYSTEMD_LIBRARIES})
    else()
        message("Systemd headers not found.")
    endif()
else()
    message("Systemd has been disabled by command line.")
endif()

target_include_directories(rest2smtp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(rest2smtp PRIVATE c_std_17)
if(ENABLE_LTO)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_property(TARGET rest2smtp PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message("LTO can be enabled only with Release variant, assuming ENABLE_LTO=OFF.")
    endif()
endif()

install(TARGETS rest2smtp DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rest2smtp.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rest2smtp)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LOCALSTATEDIR}/spool/rest2smtp)