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
#include <utility>

#include <boost/beast/http.hpp>

#include "json.h"
#include "web/websocket_client.h"
#include "web/websocket_error.h"
#include "web/websocket_server.h"

using namespace std::literals::chrono_literals;

using namespace boost::beast;

using namespace bc;

namespace
{

class WebSocketServerFixture
{
public:
  static constexpr char const *SERVER_HOST = "0.0.0.0";
  static constexpr uint16_t SERVER_PORT = 8080;

  WebSocketServerFixture()
  {
    m_server.support("/echo",
                     [](json const &data)
                     {
                       return data;
                     });

    m_server.support("/echo-fail",
                     [](json const &data)
                     {
                       throw WebSocketError("Echo failed");

                       return data;
                     });

    m_server_thread = std::thread { [this]{ m_server.run(); } };
  }

  ~WebSocketServerFixture()
  {
    m_server.stop();
    m_server_thread.join();
  }

private:
  WebSocketServer m_server { SERVER_HOST, SERVER_PORT };
  std::thread m_server_thread;
};

} // end namespace

TEST_CASE_METHOD(WebSocketServerFixture, "websocket_test", "[web]")
{
  WebSocketClient test_client { SERVER_HOST, SERVER_PORT };

  SECTION("request")
  {
    json request;
    request["target"] = "/echo";
    request["data"] = "hello";

    auto [success, answer] = test_client.send_sync(request);

    REQUIRE(success);
    CHECK(answer == "\"hello\"");
  }

  SECTION("multiple sequential requests")
  {
    enum { NUM_REQUESTS = 2 };

    for (int i = 0; i < NUM_REQUESTS; ++i) {
      json request;
      request["target"] = "/echo";
      request["data"] = "hello " + std::to_string(i);

      bool callback_run { false };

      test_client.send_async(
        request,
        [&callback_run, i](bool success, std::string const &answer)
        {
          callback_run = true;

          REQUIRE(success);
          CHECK(answer == "\"hello " + std::to_string(i) + "\"");
        });

      test_client.run();

      INFO("Request no. " + std::to_string(i + 1));
      CHECK(callback_run);
    }
  }

  SECTION("multiple parallel requests")
  {
    enum { NUM_REQUESTS = 2 };

    std::array<bool, NUM_REQUESTS> callback_run = { false };

    for (int i = 0; i < NUM_REQUESTS; ++i) {
      json request;
      request["target"] = "/echo";
      request["data"] = "hello " + std::to_string(i);

      test_client.send_async(
        request,
        [&callback_run, i](bool success, std::string const &answer)
        {
          callback_run[i] = true;

          REQUIRE(success);
          CHECK(answer == "\"hello " + std::to_string(i) + "\"");
        });
    }

    test_client.run();

    for (int i = 0; i < NUM_REQUESTS; ++i) {
      INFO("Request no. " + std::to_string(i + 1));
      CHECK(callback_run[i]);
    }
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
    request["target"] = "/echo-fail";
    request["data"] = "hello";

    auto [success, answer] = test_client.send_sync(request);

    REQUIRE(!success);
    CHECK(answer == "Echo failed");
  }

  // XXX Server timeout.

  // XXX Client timeout.
}
