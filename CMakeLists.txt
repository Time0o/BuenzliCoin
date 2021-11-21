cmake_minimum_required(VERSION 3.9)

project(BuenzliMuenzli LANGUAGES CXX)

include(CTest)

find_package(Boost 1.67.0 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)

macro(bm_config target)
  target_compile_features(${target} PRIVATE cxx_std_20)

  target_include_directories(${target} PRIVATE include)

  target_link_libraries(${target} PRIVATE
    Boost::boost
    OpenSSL::SSL
    Threads::Threads
    fmt::fmt
    nlohmann_json::nlohmann_json)
endmacro()

add_executable(node
  src/node.cc
  src/web/http_server.cc
  src/web/websocket_client.cc
  src/web/websocket_server.cc)

bm_config(node)

if(BUILD_TESTING)
  find_package(Catch2 REQUIRED)

  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Catch2/contrib)

  include(Catch)

  macro(bm_test target)
    add_executable(${target} ${ARGN})

    bm_config(${target})

    target_link_libraries(${target} PRIVATE Catch2::Catch2)

    catch_discover_tests(${target})
  endmacro()

  bm_test(http_test
    src/web/http_client.cc
    src/web/http_server.cc
    test/web/http_test.cc)

  bm_test(websocket_test
    src/web/websocket_client.cc
    src/web/websocket_server.cc
    test/web/websocket_test.cc)
endif()
