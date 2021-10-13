cmake_minimum_required(VERSION 3.6)

project(BuenzliMuenzli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)

add_executable(main main.cc)
target_link_libraries(main OpenSSL::SSL)
