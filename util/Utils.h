#pragma once

#include <csignal>
#include <functional>
#include <map>

static std::map<int, std::function<void()>> handlers_;
static void signal_handler(int sig) {
  handlers_[sig]();
}

struct Signal {
  static void signal(int sig, const std::function<void()> &handler) {
    handlers_[sig] = handler;
    ::signal(sig, signal_handler);
  }
};

class Context;

using Handler=std::function<std::string(Context*)>;
using MiddlewareHandler=std::function<void(Context*)>;