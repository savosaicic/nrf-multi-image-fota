cmake_minimum_required(VERSION 3.20.0)

set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/app0/boards/nrf9151dk_nrf9151_ns.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app0)

target_include_directories(app PRIVATE app0/include)
target_sources(app PRIVATE
  app0/src/main.c
  app0/src/gnss.c
  app0/src/temperature.c
  app0/src/ecompass.c
  app0/src/firmware_update.c
)
