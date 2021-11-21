#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>

#include <boost/beast/http.hpp>

#include "json.h"
#include "web/websocket_client.h"
#include "web/websocket_error.h"
#include "web/websocket_server.h"

using namespace std::literals::chrono_literals;

using namespace boost::beast;

using namespace bm;

namespace
{

class WebSocketServerFixture
{
public:
  static constexpr char const *SERVER_HOST = "0.0.0.0";
  static constexpr uint16_t SERVER_PORT = 8080;

  WebSocketServerFixture()
  {
    switch (fork_server())
    {
    case -1:
      throw std::runtime_error("failed to create server process");
    case 0:
      run_server();
    default:
      wait_for_server();
    };
  }

  ~WebSocketServerFixture()
  { kill_server(); }

private:
  pid_t fork_server()
  { return m_pid = fork(); }

  void run_server() const
  {
    WebSocketServer server { SERVER_HOST, SERVER_PORT };

    server.support("echo",
                   [](json const &data)
                   {
                     return data;
                   });

    server.support("echo-fail",
                   [](json const &data)
                   {
                     throw WebSocketError("Echo failed");

                     return data;
                   });

    server.run();
  }

  static void wait_for_server()
  { std::this_thread::sleep_for(1s); }

  void kill_server() const
  { kill(m_pid, SIGINT); }

  pid_t m_pid;
};

} // end namespace

TEST_CASE_METHOD(WebSocketServerFixture, "WebSocket communication works", "[websocket]")
{
  WebSocketClient test_client { SERVER_HOST, SERVER_PORT };

  SECTION("request")
  {
    json request;
    request["target"] = "echo";
    request["data"] = "hello";

    auto [success, answer] = test_client.send_sync(request);

    REQUIRE(success);
    CHECK(answer == "\"hello\"");
  }

  SECTION("multiple requests")
  {
    json request;

    bool first_callback_run { false };

    request["target"] = "echo";
    request["data"] = "first hello";

    test_client.send_async(
      request,
      [&](bool success, std::string const &answer)
      {
        first_callback_run = true;

        REQUIRE(success);
        CHECK(answer == "\"first hello\"");
      });

    bool second_callback_run { false };

    request["target"] = "echo";
    request["data"] = "second hello";

    test_client.send_async(
      request,
      [&](bool success, std::string const &answer)
      {
        second_callback_run = true;

        REQUIRE(success);
        CHECK(answer == "\"second hello\"");
      });

    test_client.run();

    REQUIRE(first_callback_run);
    REQUIRE(second_callback_run);
  }

  // XXX Invalid server URL.

  SECTION("invalid target")
  {
    json request;
    request["target"] = "invalid-target";
    request["data"] = "";

    auto [success, answer] = test_client.send_sync(request);

    REQUIRE(!success);
    CHECK(answer == "Not found");
  }

  SECTION("malformed request")
  {
    json request = "bogus";

    auto [success, answer] = test_client.send_sync(request);

    REQUIRE(!success);
    CHECK(answer == "Bad request");
  }

  SECTION("failing request")
  {
    json request;
    request["target"] = "echo-fail";
    request["data"] = "hello";

    auto [success, answer] = test_client.send_sync(request);

    REQUIRE(!success);
    CHECK(answer == "Echo failed");
  }

  // XXX Server timeout.

  // XXX Client timeout.
}
