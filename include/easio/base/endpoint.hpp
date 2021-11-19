#ifndef EASIO_BASE_ENDPOINT_HPP
#define EASIO_BASE_ENDPOINT_HPP
#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include <string_view>

namespace easio {
  namespace base {

    template <typename Protocol>
    class endpoint {
    public:
      endpoint() noexcept : data_() { data_.v4.sin_family = AF_INET; }

      endpoint(const Protocol &protocol, unsigned short port) noexcept : data_() {
        if (protocol.family() == AF_INET) {
          data_.v4.sin_family = AF_INET;
          data_.v4.sin_port = htons(port_num);
        } else {
          data_.v6.sin6_family = AF_INET6;
          data_.v6.sin6_port = htons(port_num);
        }
      }

      endpoint(const protocol_type &protocol, const std::string_view &host,
               const std::string_view &service,
               unsigned int flags = AI_ADDRCONFIG) noexcept
          : data_() {}

      bool is_v4() const noexcept { return data_.base.sa_family == AF_INET; }

    private:
      union {
        struct sockaddr base;
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
      } data_;
    };
  }  // namespace base
}  // namespace easio
#endif