cmake_minimum_required(VERSION 3.6)
project(indriclient)


add_compile_options("-Wno-deprecated-declarations")
include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/asio/include")

set(SOURCE_FILES Application.cpp Application.h Connection.cpp Connection.h)

add_executable(client ${SOURCE_FILES})
set_property(TARGET client PROPERTY CXX_STANDARD 11)
