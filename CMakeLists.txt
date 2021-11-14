cmake_minimum_required(VERSION 3.9)

project(BuenzliMuenzli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost 1.67.0 REQUIRED COMPONENTS)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(node
  src/node.cc
  src/web/http_server.cc
  src/web/websocket_client.cc
  src/web/websocket_server.cc)

target_include_directories(node PRIVATE
  include)

target_link_libraries(node PRIVATE
  Boost::boost
  OpenSSL::SSL
  Threads::Threads
  fmt::fmt
  nlohmann_json::nlohmann_json)
