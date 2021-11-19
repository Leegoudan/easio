#ifndef EASIO_BASE_RESOLVER_HPP
#define EASIO_BASE_RESOLVER_HPP
#pragma once

namespace easio {
  namespace base {
    template <typename Protocol, typename Executor>
    class resolver {
    public:
      template <typename OtherExecutor>
      struct rebind_executor {
        /// The resolver type when rebound to the specified executor.
        typedef class resolver<Protocol, OtherExecutor> other;
      };

      using endpoint = typename Protocol::endpoint;
      using results = std::vector<endpoint>;
    };
  }  // namespace base
}  // namespace easio

#endif