#ifndef EASIO_BASE_EXECUTION_CONTEXT_HPP
#define EASIO_BASE_EXECUTION_CONTEXT_HPP
#pragma once

#include <memory>

#include "base/noncopyable.hpp"

namespace easio {
  namespace base {

    class service_registry;

    class execution_context : private noncopyable {
    public:
      class id;
      class service;

      template <typename Service>
      friend Service &use_service(execution_context &e);

      template <typename Service, typename... Args>
      friend Service &make_service(execution_context &e, Args &&...args);

      template <typename Service>
      friend void add_service(execution_context &e,
                              std::shared_ptr<Service> svc);

      template <typename Service>
      friend bool has_service(execution_context &e);

      execution_context();
      ~execution_context();

    protected:
      void shutdown();
      void destroy();

    private:
      std::shared_ptr<service_registry> service_registry_ptr_;
    };

    class execution_context::id : private noncopyable {
    public:
      execution_context::id() = default;
    };

    class execution_context::service : private noncopyable {
    public:
      execution_context &context() { return owner_; }

    protected:
      inline service(execution_context &owner);
      inline virtual ~service();

    private:
      virtual void shutdown() = 0;
      friend class service_registry;
      struct key {
        key() : type_info_(nullptr), id_(nullptr) {}
        const std::type_info *type_info_;
        const execution_context::id *id_;
      } key_;

      execution_context &owner_;
      std::shared_ptr<service> next_;
    };

    template <typename Type>
    class service_id : public execution_context::id {};

    template <typename Type>
    class execution_context_service : public execution_context::service {
    public:
      static service_id<Type> id;

      execution_context_service(execution_context &e)
          : execution_context::service(e) {}
    };

    template <typename Type>
    service_id<Type> execution_context_service<Type>::id;
  }  // namespace base
}  // namespace easio

#include "base/impl/execution_context.ipp"

#endif