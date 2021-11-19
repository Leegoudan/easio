#ifndef EASIO_TCP_HPP
#define EASIO_TCP_HPP
#pragma once

#include "base/endpoint.hpp"

namespace easio {
  class tcp {
public:
    using endpoint = base::endpoint<tcp>;

    static tcp v4() noexcept {
      return tcp(AF_INET);
    }

    static tcp v6() noexcept {
      return tcp(AF_INET6);
    }

    int family() const noexcept {
      return family_;
    }

private:
    explicit tcp(int family) noexcept : family_(family) {}
    int family_;
  };
}  // namespace easio
#endif