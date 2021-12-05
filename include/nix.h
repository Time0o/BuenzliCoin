#pragma once

#include <cstring>

#include <signal.h>

namespace bc::nix
{

void on_termination(void (*func)(int))
{
  struct sigaction sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = func;
  sa.sa_flags = SA_RESETHAND;
  sigaction(SIGTERM, &sa, nullptr);
}

} // end namespace bc::nix
