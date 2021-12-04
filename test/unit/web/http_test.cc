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
    m_server.support("/hello",
                     HTTPServer::method::get,
                     [](json const &)
                     {
                       json answer = "hello";

                       return std::make_pair(HTTPServer::status::ok, answer);
                     });

    m_server.support("/echo",
                     HTTPServer::method::post,
                     [](json const &data)
                     { return std::make_pair(HTTPServer::status::ok, data); });

    m_server.support("/echo-fail",
                     HTTPServer::method::post,
                     [](json const &data)
                     {
                       throw HTTPError(HTTPServer::status::internal_server_error, "Echo failed");

                       return std::make_pair(HTTPServer::status::ok, data);
                     });

    m_server_thread = std::thread { [this]{ m_server.run(); } };
  }

  ~HTTPServerFixture()
  {
    m_server.stop();
    m_server_thread.join();
  }

private:
  HTTPServer m_server { SERVER_HOST, SERVER_PORT };
  std::thread m_server_thread;
};

} // end namespace

TEST_CASE_METHOD(HTTPServerFixture, "http_test", "[web]")
{
  HTTPClient test_client { SERVER_HOST, SERVER_PORT };

  SECTION("GET request")
  {
    auto [status, answer] = test_client.send_sync("/hello", HTTPServer::method::get);

    REQUIRE(status == HTTPServer::status::ok);

    CHECK(answer == "\"hello\"");
  }

  SECTION("POST request")
  {
    json data = "echo";

    auto [status, answer] = test_client.send_sync("/echo", HTTPServer::method::post, data);

    REQUIRE(status == HTTPServer::status::ok);

    CHECK(answer == "\"echo\"");
  }

  SECTION("multiple requests")
  {
    {
      auto [status, _] = test_client.send_sync("/hello", HTTPServer::method::get);

      CHECK(status == HTTPServer::status::ok);
    }

    {
      auto [status, _] = test_client.send_sync("/hello", HTTPServer::method::get);

      CHECK(status == HTTPServer::status::ok);
    }
  }

  SECTION("invalid target")
  {
    auto [status, answer] = test_client.send_sync("/invalid-target", HTTPServer::method::get);

    REQUIRE(status == HTTPServer::status::not_found);
    CHECK(answer == "File not found");
  }

  SECTION("invalid method")
  {
    auto [status, answer] = test_client.send_sync("/hello", HTTPServer::method::post);

    REQUIRE(status == HTTPServer::status::bad_request);
    CHECK(answer == "Invalid request method 'POST'");
  }

  // XXX Invalid content type.

  // XXX Malformed request.

  SECTION("failing request")
  {
    json data = "echo";

    auto [status, answer] = test_client.send_sync("/echo-fail", HTTPServer::method::post, data);

    REQUIRE(status == HTTPServer::status::internal_server_error);
    CHECK(answer == "Echo failed");
  }
}
