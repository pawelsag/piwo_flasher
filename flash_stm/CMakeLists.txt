cmake_minimum_required(VERSION 3.10)
project(FLASH_STM)

set(CMAKE_CXX_STANDARD 20)

set (TARGET_NAME flash_stm)

add_executable(${TARGET_NAME} flash_stm.cc)

target_include_directories(${TARGET_NAME} PRIVATE ../Proto)
