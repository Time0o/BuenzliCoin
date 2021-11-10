#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "blockchain.h"
#include "http_server.h"

namespace bm
{

class Node
{
public:
  explicit Node(std::string const &addr, uint16_t port)
  {
    HTTPServer server { addr, port };
    server.run(); // XXX
  }

#if 0
  {
    create_listener("list-blocks", http::methods::GET,
                    [this](http::http_request request)
                    { handle_list_blocks(request); });

    create_listener("add-block", http::methods::POST,
                    [this](http::http_request request)
                    { handle_add_block(request); });
  }

  ~Node()
  {
    for (auto &[_, listener] : m_listeners)
      listener.close();
  }

  void run()
  {
    for (auto &[_, listener] : m_listeners)
      listener.open();
  }

  static void block()
  {
    std::this_thread::sleep_until(
      std::chrono::time_point<std::chrono::system_clock>::max());
  }

private:
  template<typename FUNC>
  void create_listener(std::string const &path,
                       http::method const &method,
                       FUNC &&handler)
  {
    auto uri { m_host + "/" + path };

    listener::http_listener listener { uri };
    listener.support(method, std::forward<FUNC>(handler));

    m_listeners.emplace(uri, std::move(listener));
  }

  void handle_list_blocks(http::http_request request)
  {
    auto answer { m_blockchain.to_json() };

    request.reply(http::status_codes::OK, answer);
  }

  void handle_add_block(http::http_request request)
  {
    auto status { http::status_codes::OK };
    web::json::value answer;

    auto action = [&](pplx::task<web::json::value> const &task)
    {
      std::string data;

      try {
        auto j { task.get() };
        data = j["data"].as_string();

        m_blockchain.append(data);

      } catch (std::exception const &e) {
        status = http::status_codes::BadRequest;
        answer["message"] = web::json::value(e.what());
      }
    };

    request.extract_json()
           .then(action)
           .wait();

    request.reply(status, answer);
  }

  Blockchain<> m_blockchain;

  std::string m_host;
  std::unordered_map<std::string, listener::http_listener> m_listeners;
#endif
};

} // end namespace bm
