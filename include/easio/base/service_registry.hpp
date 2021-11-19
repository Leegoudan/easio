#ifndef EASIO_BASE_SERVICE_REGISTRY_HPP
#define EASIO_BASE_SERVICE_REGISTRY_HPP
#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "easio/base/execution_context.hpp"
#include "easio/base/noncopyable.hpp"

namespace easio {
namespace base {
template <typename T>
class typeid_wrapper {};

class service_registry : private noncopyable {
    using context_service_ptr = std::shared_ptr<execution_context::service>;
    using context_ptr = std::shared_ptr<execution_context>;
    using service_key = execution_context::service::key;

   public:
    inline service_registry(execution_context &owner);
    inline void shutdown_services();
    inline void destroy_services();

    template <typename Service>
    Service &use_service();

    template <typename Service>
    void add_service(std::shared_ptr<Service> new_service_ptr);

    template <typename Service>
    bool has_service() const;

    template <typename Service>
    static void init_key(
        service_key &key,
        typename std::enable_if<std::is_base_of<typename Service::key_type,
                                                Service>::value>::type *);

    inline static bool keys_match(const service_key &lkey,
                                  const service_key &rkey);

   private:
    mutable std::mutex mutex_;
    execution_context &owner_;
    context_service_ptr first_service_ptr_;
};
}  // namespace base
}  // namespace easio

#endif