project(Projekt_PIWO_Module_Flasher C CXX ASM)
cmake_minimum_required(VERSION 3.5.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

option(CONFIG_BUILD_TESTS "Enable tests" OFF)
option(CONFIG_SIMULATION "Enable simulation" OFF)

# disable warning no-regsiter only for cxx files
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

  include_directories(Proto)
if(CONFIG_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
else()
  set(TARGET_NAME flasher)
  include(pico-sdk/pico_sdk_init.cmake)

  # initialize the Raspberry Pi Pico SDK
  pico_sdk_init()

  add_executable(${TARGET_NAME}
    App/rpi_main.cpp
    App/usb_descriptors.c
    App/flasher.cpp
    App/iface.c
    rp2040/rp2040_iface.cpp
  )

  target_include_directories(${TARGET_NAME} PUBLIC App)

  target_link_libraries(${TARGET_NAME} pico_stdlib tinyusb_device tinyusb_board hardware_uart)

  # create map/bin/hex/uf2 file in addition to ELF.
  pico_add_extra_outputs(${TARGET_NAME})

endif()
