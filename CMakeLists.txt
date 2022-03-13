project(Projekt_PIWO_Module_Flasher C CXX ASM)
cmake_minimum_required(VERSION 3.5.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

option(CONFIG_BUILD_TESTS "Enable tests" OFF)

# disable warning no-regsiter only for cxx files
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

if(NOT DEFINED PIWO_PC_FILE_NAME)
  set(PIWO_PC_FILE_NAME piwo)
  set(PIWO_PC_VERSION_REQUIRED "1.6")
else()
  set(PIWO_PC_VERSION_REQUIRED "0.1")
endif(NOT DEFINED PIWO_PC_FILE_NAME)

set(TARGET_NAME module)

# set proper files name
SET(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F303VCTx_FLASH.ld)
SET(BOOT_FILE  ${CMAKE_SOURCE_DIR}/startup_stm32f303xc.s)

include_directories(Proto)
include_directories(Core/Inc)
include_directories(USB_DEVICE/App)
include_directories(USB_DEVICE/Target)
include_directories(Middlewares/Inc)
include_directories(Drivers/STM32F3xx_HAL_Driver/Inc)
include_directories(Drivers/STM32F3xx_HAL_Driver/Inc/Legacy)
include_directories(Drivers/CMSIS/Device/ST/STM32F3xx/Include)
include_directories(Drivers/CMSIS/Include)

set(ELF_FILE ${PROJECT_NAME}.elf)
set(HEX_FILE ${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_NAME}.bin)

set(HEX_FILEPATH ${CMAKE_BINARY_DIR}/${HEX_FILE})
set(BIN_FILEPATH ${CMAKE_BINARY_DIR}/${BIN_FILE})
set(ELF_FILEPATH ${CMAKE_BINARY_DIR}/${ELF_FILE})

set(VERSION_MAJOR "6")
set(VERSION_MINOR "0")

set(PIWO "${PROJECT_NAME}.${VERSION_MAJOR}.${VERSION_MINOR}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(${PIWO_PC_FILE_NAME} REQUIRED ${PIWO_PC_FILE_NAME}>=${PIWO_PC_VERSION_REQUIRED})

file(GLOB_RECURSE MAIN_SOURCES ${CMAKE_SOURCE_DIR}/Core/Src/*.c)
file(GLOB_RECURSE MIDDLEWARES_SOURCES ${CMAKE_SOURCE_DIR}/Middlewares/Src/*.c*)
file(GLOB_RECURSE USB_APP_SOURCES ${CMAKE_SOURCE_DIR}/USB_DEVICE/App/*.c*)
file(GLOB_RECURSE USB_TARGET_SOURCES ${CMAKE_SOURCE_DIR}/USB_DEVICE/Target/*.c*)
file(GLOB_RECURSE HAL_SRC ${CMAKE_SOURCE_DIR}/Drivers/STM32F3xx_HAL_Driver/Src/*.c*)

set(PIWO_SOURCES
  ${USB_APP_SOURCES}
  ${USB_TARGET_SOURCES}
  ${HAL_SRC}
  ${MIDDLEWARES_SOURCES}
  ${MAIN_SOURCES}
  ${BOOT_FILE}
)

add_executable(${TARGET_NAME}
  ${PIWO_SOURCES}
)

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
