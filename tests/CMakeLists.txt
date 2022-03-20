cmake_minimum_required(VERSION 3.10)

set (TEST_NAME proto_test)

set (TESTS
  flasher_init_test.cc
  flasher_frame_test.cc
  flasher_reset_test.cc
  flasher_reset_done_test.cc
  flasher_msg_test.cc
)

find_library(libgtest gtest REQUIRED)
find_library(libgtestmain gtest_main REQUIRED)
find_library(libgmock gmock REQUIRED)
find_library(libpthread pthread REQUIRED)

add_executable(${TEST_NAME} ${TESTS})

target_link_libraries(
  ${TEST_NAME}
  ${libgtestmain}
  ${libgtest}
  ${libgmock}
  ${libpthread}
)

