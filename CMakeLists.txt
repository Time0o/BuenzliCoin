cmake_minimum_required(VERSION 3.9)

project(BuenzliCoin LANGUAGES CXX)

option(BUENZLI_BUILD_TESTS "Build BuenzliCoin unit and integration tests" ON)
option(BUENZLI_BUILD_BWALLET "Build 'buenzli' script" ON)

find_package(Boost COMPONENTS log_setup log program_options REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(tomlplusplus REQUIRED)

macro(bm_config target)
  target_compile_features(${target} PRIVATE cxx_std_20)

  target_include_directories(${target} PRIVATE include)

  target_link_libraries(${target} PRIVATE
    Boost::boost
    Boost::log_setup
    Boost::log
    Boost::program_options
    OpenSSL::SSL
    Threads::Threads
    fmt::fmt
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus)
endmacro()

add_executable(bnode
  src/config.cc
  src/log.cc
  src/main.cc
  src/node.cc
  src/transaction.cc
  src/web/http_server.cc
  src/web/websocket_client.cc
  src/web/websocket_server.cc)

bm_config(bnode)

target_compile_definitions(bnode PRIVATE -DPROOF_OF_WORK -DTRANSACTIONS)

if (BUENZLI_BUILD_TESTS AND BUILD_TESTING)
  add_executable(bnode_naive
    src/config.cc
    src/log.cc
    src/main.cc
    src/node.cc
    src/web/http_server.cc
    src/web/websocket_client.cc
    src/web/websocket_server.cc)

  bm_config(bnode_naive)

  add_executable(bnode_pow
    src/config.cc
    src/log.cc
    src/main.cc
    src/node.cc
    src/web/http_server.cc
    src/web/websocket_client.cc
    src/web/websocket_server.cc)

  target_compile_definitions(bnode_pow PRIVATE -DPROOF_OF_WORK)

  bm_config(bnode_pow)

  add_executable(bnode_trans
    src/config.cc
    src/log.cc
    src/main.cc
    src/node.cc
    src/transaction.cc
    src/web/http_server.cc
    src/web/websocket_client.cc
    src/web/websocket_server.cc)

  bm_config(bnode_trans)

  target_compile_definitions(bnode_trans PRIVATE -DTRANSACTIONS)

  # Include Catch2.
  include(CTest)

  find_package(Catch2 REQUIRED)

  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Catch2/contrib)

  include(Catch)

  # Build unit tests.
  macro(bm_unit_test target)
    add_executable(${target} ${ARGN})

    bm_config(${target})

    target_link_libraries(${target} PRIVATE Catch2::Catch2)

    catch_discover_tests(${target})
  endmacro()

  bm_unit_test(difficulty_test
    test/unit/difficulty_test.cc)

  bm_unit_test(digest_test
    test/unit/crypto/digest_test.cc)

  bm_unit_test(keypair_test
    test/unit/crypto/keypair_test.cc)

  bm_unit_test(http_test
    src/web/http_client.cc
    src/web/http_server.cc
    test/unit/web/http_test.cc)

  bm_unit_test(websocket_test
    src/web/websocket_client.cc
    src/web/websocket_server.cc
    test/unit/web/websocket_test.cc)

  # Build Python bindings
  find_package(pybind11 REQUIRED)
  find_package(pybind11_json REQUIRED)

  pybind11_add_module(bc src/python.cc)

  target_link_libraries(bc PRIVATE pybind11_json::pybind11_json)

  bm_config(bc)

  # Build integration tests
  find_program(PYTHON python3 REQUIRED)

  macro(bm_integration_test target)
    add_test(
      NAME ${target}
      COMMAND ${CMAKE_COMMAND} -E env NODE_NAIVE=$<TARGET_FILE:bnode_naive>
                                      NODE_POW=$<TARGET_FILE:bnode_pow>
                                      NODE_TRANS=$<TARGET_FILE:bnode_trans>
                                      PYTHONPATH=$<TARGET_FILE_DIR:bc>
              ${PYTHON} ${ARGN}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endmacro()

  bm_integration_test(blockchain_test
    test/integration/blockchain_test.py)

  bm_integration_test(proof_of_work_test
    test/integration/proof_of_work_test.py)

  bm_integration_test(transaction_test
    test/integration/transaction_test.py)
endif()

# Build 'bwallet' script.
if (BUENZLI_BUILD_BWALLET)
  find_program(GO go REQUIRED)

  set(BWALLET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/bwallet/bwallet.go")
  set(BWALLET_BIN "${CMAKE_CURRENT_BINARY_DIR}/bwallet")

  add_custom_command(
    OUTPUT "${BWALLET_BIN}"
    COMMAND ${GO} build -o "${BWALLET_BIN}" "${BWALLET_SRC}"
    DEPENDS "${BWALLET_SRC}")

  add_custom_target(bwallet_script ALL
    DEPENDS "${BWALLET_BIN}")
endif()
