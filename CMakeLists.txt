cmake_minimum_required(VERSION 3.9)

project(BuenzliMuenzli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(fmt REQUIRED)

add_executable(main main.cc)
target_link_libraries(main OpenSSL::SSL cpprestsdk::cpprest fmt::fmt)
