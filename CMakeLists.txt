cmake_minimum_required(VERSION 3.20)
project(multipart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(multipart INTERFACE)
add_library(multipart::multipart ALIAS multipart)

target_include_directories(multipart INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if (MSVC)
  target_compile_options(multipart INTERFACE /W4 /permissive-)
else()
  target_compile_options(multipart INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
enable_testing()

add_executable(multipart_basic_test tests/test_basic.cpp)
target_link_libraries(multipart_basic_test PRIVATE multipart::multipart)
add_test(NAME multipart.basic COMMAND multipart_basic_test)
