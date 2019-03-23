cmake_minimum_required (VERSION 2.6)
project (anago)
add_definitions (-DANAGO=1 -DDEBUG=1)
add_compile_options(-Wall -Werror)

find_package(PkgConfig REQUIRED QUIET)
pkg_search_module(SQUIRREL REQUIRED squirrel3)
pkg_search_module(USB REQUIRED libusb)
pkg_search_module(NCURSES REQUIRED ncurses)

add_custom_command(OUTPUT anago.1
    COMMAND txt2man 
    ARGS -t anago -r anago-0.6.0l -s 1 -v "Anago/Unagi" anago.1_src > anago.1
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT Regenerating makefile
    DEPENDS anago.1_src )
add_custom_target(manual DEPENDS anago.1)

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/build_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/build_config.h"
  )

link_libraries(
    ${SQUIRREL_LDFLAGS}
    ${USB_LDFLAGS}
    ${NCURSES_LDFLAGS}
)
include_directories(
    ${SQUIRREL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${USB_INCLUDE_DIRS}
    ${NCURSES_INCLUDE_DIRS}
    kazzo
)

add_executable(anago
    anago.c
    crc32.c
    file.c
    flash_device.c
    header.c
    memory_manage.c
    progress.c
    reader_dummy.c
    reader_kazzo.c
    script_common.c
    script_dump.c
    script_flash.c
    squirrel_wrap.c
    usb_device.c
)

install(TARGETS anago
    RUNTIME DESTINATION bin )
install(FILES anago.1
    DESTINATION share/man/man1)
install(DIRECTORY scripts/ 
    DESTINATION share/anago)
