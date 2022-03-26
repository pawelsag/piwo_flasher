cmake_minimum_required(VERSION 3.10)
project(FLASH_STM)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(
  SPDLOG_FMT_EXTERNAL
)

find_package(PkgConfig REQUIRED)
find_package(spdlog CONFIG)
pkg_check_modules(fmt REQUIRED fmt)

set (TARGET_NAME flash_stm)

add_executable(${TARGET_NAME} flash_stm.cc)

target_include_directories(${TARGET_NAME} PRIVATE
  ../Proto
  ${fmt_INCLUDE_DIRS}
  ${spdlog_INCLUDE_DIRS}
)

target_link_directories(${TARGET_NAME} PRIVATE
  ${spdlog_LIBRARY_DIRS}
  ${fmt_LIBRARY_DIRS}
)

target_link_libraries("${TARGET_NAME}" PRIVATE
  ${fmt_LIBRARIES}
  spdlog
)
