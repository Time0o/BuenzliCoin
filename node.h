#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include <cpprest/http_listener.h>
#include <cpprest/json.h>

#include "blockchain.h"

// XXX source file
namespace http = web::http;
namespace listener = web::http::experimental::listener;

namespace bm
{

class Node
{
public:
  explicit Node(std::string const &host) // XXX specify port
  : m_host(host)
  {
    create_listener("list-blocks", http::methods::GET,
                    [this](http::http_request request)
                    { handle_list_blocks(request); });

    create_listener("add-block", http::methods::POST,
                    [this](http::http_request request)
                    { handle_add_block(request); });

    m_blockchain.append("hello"); // XXX
    m_blockchain.append("world"); // XXX
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

  static void handle_add_block(http::http_request request)
  {
    std::cout << "TODO: add block" << std::endl; // XXX
  }

  Blockchain<> m_blockchain;

  std::string m_host;
  std::unordered_map<std::string, listener::http_listener> m_listeners;
};

} // end namespace bm
