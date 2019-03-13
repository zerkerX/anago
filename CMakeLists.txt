# http://alpha/cgi-bin/dwww/usr/share/doc/cmake-data/html/command/add_library.html?highlight=library#imported-libraries

cmake_minimum_required (VERSION 2.6)
project (anago)
add_definitions (-DANAGO=1 -DDEBUG=1)
add_compile_options(-Wall -Werror)

find_package(PkgConfig REQUIRED QUIET)
pkg_search_module(SQUIRREL REQUIRED squirrel3)
pkg_search_module(USB REQUIRED libusb)

link_libraries(
    ${SQUIRREL_LDFLAGS}
    ${USB_LDFLAGS}
)
include_directories(
    ${SQUIRREL_INCLUDE_DIRS}
    ${USB_INCLUDE_DIRS}
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
install(DIRECTORY scripts/ DESTINATION share/anago)
