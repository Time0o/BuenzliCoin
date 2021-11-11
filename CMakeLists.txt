cmake_minimum_required(VERSION 3.9)

project(BuenzliMuenzli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)
find_package(Boost 1.67.0 REQUIRED COMPONENTS)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

add_executable(main main.cc http_server.cc)
target_link_libraries(main OpenSSL::SSL Boost::boost Threads::Threads fmt::fmt)
