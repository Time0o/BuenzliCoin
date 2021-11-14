#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>

#include <boost/beast/http.hpp>

#include "json.h"
#include "web/http_client.h"
#include "web/http_error.h"
#include "web/http_server.h"

using namespace std::literals::chrono_literals;

using namespace boost::beast;

using namespace bm;

namespace
{

class HTTPServerFixture
{
public:
  static constexpr char const *SERVER_HOST = "0.0.0.0";
  static constexpr uint16_t SERVER_PORT = 8080;

  HTTPServerFixture()
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

  ~HTTPServerFixture()
  { kill_server(); }

private:
  pid_t fork_server()
  { return m_pid = fork(); }

  void run_server() const
  {
    HTTPServer server { SERVER_HOST, SERVER_PORT };

    server.support("hello",
                   HTTPServer::method::get,
                   [](json const &)
                   {
                     json answer = "hello";

                     return std::make_pair(HTTPServer::status::ok, answer);
                   });

    server.support("echo",
                   HTTPServer::method::post,
                   [](json const &data)
                   { return std::make_pair(HTTPServer::status::ok, data); });

    server.support("echo-fail",
                   HTTPServer::method::post,
                   [](json const &data)
                   {
                     throw HTTPError(HTTPServer::status::internal_server_error, "Echo failed");

                     return std::make_pair(HTTPServer::status::ok, data);
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

TEST_CASE_METHOD(HTTPServerFixture, "HTTP communication works", "[http]")
{
  HTTPClient test_client { SERVER_HOST, SERVER_PORT };

  SECTION("GET request")
  {
    auto [status, answer] = test_client.send_sync("hello", HTTPServer::method::get);

    REQUIRE(status == HTTPServer::status::ok);

    CHECK(answer == "\"hello\"");
  }

  SECTION("POST request")
  {
    json data = "echo";

    auto [status, answer] = test_client.send_sync("echo", HTTPServer::method::post, data);

    REQUIRE(status == HTTPServer::status::ok);

    CHECK(answer == "\"echo\"");
  }

  SECTION("multiple requests")
  {
    {
      auto [status, _] = test_client.send_sync("hello", HTTPServer::method::get);

      CHECK(status == HTTPServer::status::ok);
    }

    {
      auto [status, _] = test_client.send_sync("hello", HTTPServer::method::get);

      CHECK(status == HTTPServer::status::ok);
    }
  }

  SECTION("invalid target")
  {
    auto [status, answer] = test_client.send_sync("invalid-target", HTTPServer::method::get);

    REQUIRE(status == HTTPServer::status::not_found);
    CHECK(answer == "File not found");
  }

  SECTION("invalid method")
  {
    auto [status, answer] = test_client.send_sync("hello", HTTPServer::method::post);

    REQUIRE(status == HTTPServer::status::bad_request);
    CHECK(answer == "Invalid request method 'POST'");
  }

  // XXX Invalid content type.

  // XXX Malformed request.

  SECTION("failing request")
  {
    json data = "echo";

    auto [status, answer] = test_client.send_sync("echo-fail", HTTPServer::method::post, data);

    REQUIRE(status == HTTPServer::status::internal_server_error);
    CHECK(answer == "Echo failed");
  }
}
