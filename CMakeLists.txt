project(Projekt_PIWO_Module_Flasher C CXX ASM)
cmake_minimum_required(VERSION 3.5.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

option(CONFIG_BUILD_TESTS "Enable tests" OFF)
option(CONFIG_SIMULATION "Enable simulation" OFF)
set(BOARD "STM32F303" CACHE STRING "Board selector for which the application will be build (default: stm32f303) supported boards: stm32f303, stm32f103")

# disable warning no-regsiter only for cxx files
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

  include_directories(Proto)
if(CONFIG_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
else()
  set(TARGET_NAME flasher)
  string(TOLOWER ${BOARD} BOARD_DIR)
  add_subdirectory(${BOARD_DIR})

  include_directories(App)
  include_directories(${target_INCLUDE_DIRS})

  set(ELF_FILE ${PROJECT_NAME}.elf)
  set(HEX_FILE ${PROJECT_NAME}.hex)
  set(BIN_FILE ${PROJECT_NAME}.bin)

  set(HEX_FILEPATH ${CMAKE_BINARY_DIR}/${HEX_FILE})
  set(BIN_FILEPATH ${CMAKE_BINARY_DIR}/${BIN_FILE})
  set(ELF_FILEPATH ${CMAKE_BINARY_DIR}/${ELF_FILE})

  file(GLOB_RECURSE USB_APP_SOURCES ${CMAKE_SOURCE_DIR}/App/*.c*)

  set(PIWO_SOURCES
    ${USB_APP_SOURCES}
    ${target_SOURCES}
    ${BOOT_FILE}
  )

  add_executable(${TARGET_NAME}
    ${PIWO_SOURCES}
  )

  if(CONFIG_SIMULATION)
    target_compile_definitions(${TARGET_NAME} PUBLIC WITH_SIMULATION)
  endif()

  target_compile_options(${TARGET_NAME} BEFORE PUBLIC ${MPU_FLAGS} ${VFP_FLAGS} ${COMMON_FLAGS} ${DIAGNOSTIC_FLAGS})

  target_compile_definitions(${TARGET_NAME} PUBLIC ${SDK_FLAGS})

  target_link_options(${TARGET_NAME} BEFORE PUBLIC ${MPU_FLAGS} ${VFP_FLAGS} -specs=nano.specs -specs=nosys.specs ${COMMON_FLAGS} -T${LINKER_SCRIPT} -Wl,--gc-sections)

  set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${ELF_FILE})

  if(DEFINED PIWO_PC_FILE_NAME)
    set(piwo_INCLUDE_DIRS ${${PIWO_PC_FILE_NAME}_INCLUDEDIR})
  endif(DEFINED PIWO_PC_FILE_NAME)

  target_include_directories(${TARGET_NAME} PRIVATE
    ${piwo_INCLUDE_DIRS}
  )

  target_link_directories(${TARGET_NAME} PRIVATE
    ${piwo_LIBRARY_DIRS}
  )

  target_link_libraries(${TARGET_NAME} PRIVATE
    nosys
    m
    c)

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                     COMMENT "Target size: ${ELF_FILE}"
                     COMMAND ${CMAKE_OBJECT_SIZE} ${PROJECT_NAME}.elf)

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                     COMMENT "Building: ${HEX_FILE}"
                     COMMAND ${CMAKE_OBJECT_COPY} -Oihex $<TARGET_FILE:${TARGET_NAME}> ${HEX_FILEPATH})

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                     COMMENT "Building: ${BIN_FILE}"
                     COMMAND ${CMAKE_OBJECT_COPY} -Obinary $<TARGET_FILE:${TARGET_NAME}> ${BIN_FILEPATH})

  add_custom_target(flash-stm
                    COMMENT "FLASH_STM"
                    COMMAND st-flash write ${BIN_FILEPATH} 0x8000000
                    DEPENDS ${TARGET_NAME})
endif()
