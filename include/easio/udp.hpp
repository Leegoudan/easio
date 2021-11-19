#ifndef EASIO_UDP_HPP
#define EASIO_UDP_HPP
#pragma once

#include "base/endpoint.hpp"

namespace easio {
  class udp {
public:
    typedef base::endpoint<udp> endpoint;
    static udp v4() noexcept {
      return udp(AF_INET);
    }

    static udp v6() noexcept {
      return udp(AF_INET6);
    }

private:
    explicit udp(int family) noexcept : family_(family) {}
    int family_;
  };
}  // namespace easio
#endif