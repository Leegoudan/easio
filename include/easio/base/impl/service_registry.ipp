#ifndef EASIO_BASE_SERVICE_REGISTRY_IPP
#define EASIO_BASE_SERVICE_REGISTRY_IPP
#pragma once

#include "base/service_registry.hpp"

namespace easio {
namespace base {
service_registry::service_registry(execution_context& owner) : owner_(owner) {}

void service_registry::shutdown_services() {
    context_service_ptr service_ptr = first_service_ptr_;
    while (service_ptr) {
        service_ptr->shutdown();
        service_ptr = service_ptr->next_;
    }
}

void service_registry::destroy_services() {
    while (first_service_ptr_) {
        first_service_ptr_ = first_service_ptr_->next_;
    }
}

template <typename Service>
Service& service_registry::use_service() {
    service_key key;
    init_key<Service>(key, 0);

    // check
    std::unique_lock<std::mutex> lock(mutex_);
    context_service_ptr service_ptr = first_service_ptr_;
    while (service_ptr) {
        if (keys_match(service_ptr->key_, key))
            return *service_ptr;
        service_ptr = service_ptr->next_;
    }

    // create new one
    lock.unlock();
    context_service_ptr new_service_ptr(new Service(owner_));
    new_service_ptr->key_ = key;

    // double-check
    lock.lock();
    service_ptr = first_service_ptr_;
    while (service_ptr) {
        if (keys_match(service_ptr->key_, key))
            return *service_ptr;
        service_ptr = service_ptr->next_;
    }

    // create successfully and add to registry
    new_service_ptr->next_ = first_service_ptr_;
    first_service_ptr_ = new_service_ptr;
    return *first_service_ptr_;
}

template <typename Service>
void service_registry::add_service(std::shared_ptr<Service> new_service_ptr) {
    service_key key;
    init_key<Service>(key, 0);

    if (&owner_ != &new_service_ptr->context())
        throw std::logic_error("Invalid service owner.");

    // check
    std::lock_guard<std::mutex> lock(mutex_);
    context_service_ptr service_ptr = first_service_ptr_;
    while (service_ptr) {
        if (keys_match(service_ptr->key_, key))
            throw std::logic_error("Service already exists.");
        service_ptr = service_ptr->next_;
    }

    // add
    new_service_ptr->key_ = key;
    new_service_ptr->next_ = first_service_ptr_;
    first_service_ptr_ = new_service_ptr;
}

template <typename Service>
bool service_registry::has_service() const {
    service_key key;
    init_key<Service>(key, 0);

    std::lock_guard<std::mutex> lock(mutex_);
    context_service_ptr service_ptr = first_service_ptr_;
    while (service_ptr) {
        if (keys_match(service_ptr->key_, key))
            return true;
        service_ptr = service_ptr->next_;
    }

    return false;
}

template <typename Service>
void service_registry::init_key(
    service_key& key,
    typename std::enable_if<
        std::is_base_of<typename Service::key_type, Service>::value>::type*) {
    key.type_info_ = &typeid(typeid_wrapper<Service>);
    key.id_ = 0;
}

bool service_registry::keys_match(const service_key& lkey,
                                  const service_key& rkey) {
    if (lkey.id_ && rkey.id_ && lkey.id_ == rkey.id_)
        return true;
    if (lkey.type_info_ && rkey.type_info_ &&
        *lkey.type_info_ == *rkey.type_info_)
        return true;
    return false;
}

}  // namespace base
}  // namespace easio

#endif