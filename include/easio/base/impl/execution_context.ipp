#ifndef EASIO_BASE_EXECUTION_CONTEXT_IPP
#define EASIO_BASE_EXECUTION_CONTEXT_IPP
#pragma once

#include "base/execution_context.hpp"
#include "base/service_registry.hpp"

namespace easio
{
  namespace base
  {

    template <typename Service>
    Service &use_service(execution_context &e)
    {
      //check
      (void)static_cast<execution_context::service*>(static_cast<Service*>(0));
      
      return e.service_registry_ptr_->template use_service<Service>();
    }

    template <typename Service, typename... Args>
    Service &make_service(execution_context &e, Args &&...args)
    {
        std::shared_ptr<Service> svc(new Service(e, (Args&&)(args)...));
        e.service_registry_ptr_->template add_service<Service>(svc);
        Service& result = *svc;
        return result;
    }

    template <typename Service>
    void add_service(execution_context &e, std::shared_ptr<Service> svc)
    {
      // check
      (void)static_cast<execution_context::service*>(static_cast<Service*>(0));

      e.service_registry_ptr_->template add_service<Service>(svc);
    }

    template <typename Service>
    bool has_service(execution_context &e)
    {
      // check
      (void)static_cast<execution_context::service*>(static_cast<Service*>(0));

      return e.service_registry_ptr_->template has_service<Service>();
    }

    execution_context::execution_context()
        : service_registry_ptr_(std::make_shared<service_registry>(*this))
    {
    }

    execution_context::~execution_context()
    {
      shutdown();
      destroy();
    }

    void execution_context::shutdown()
    {
      service_registry_ptr_->shutdown_services();
    }

    void execution_context::destroy()
    {
      service_registry_ptr_->destroy_services();
    }

    execution_context::service::service(execution_context &owner)
        : owner_(owner)
    {
    }

    execution_context::service::~service()
    {
    }

  } // base
} // easio

#endif