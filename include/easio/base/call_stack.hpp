#ifndef EASIO_BASE_CALL_STACK_HPP
#define EASIO_BASE_CALL_STACK_HPP
#pragma once

#include "base/noncopyable.hpp"

namespace easio {
  namespace base {

    template <typename Key, typename Value = unsigned char>
    class call_stack {
    public:
      class context : private noncopyable {
      public:
        explicit context(Key* k)
            : key_(k), next_(call_stack<Key, Value>::top_) {
          value_ = reinterpret_cast<Value*>(this);
          call_stack<Key, Value>::top_ = this;
        }

        context(Key* k, Value& v)
            : key_(k), value_(&v), next_(call_stack<Key, Value>::top_) {
          call_stack<Key, Value>::top_ = this;
        }

        ~context() {
          call_stack<Key, Value>::top_ = next_;
        }

        Value* next_by_key() const {
          context* elem = next_;
          while (elem) {
            if (elem->key_ == key_)
              return elem->value_;
            elem = elem->next_;
          }
          return nullptr;
        }

      private:
        friend class call_stack<Key, Value>;

        Key* key_;
        Value* value_;
        context* next_;
      };

      friend class context;

      static Value* contains(Key* k) {
        context* elem = top_;
        while (elem) {
          if (elem->key_ == k)
            return elem->value_;
          elem = elem->next_;
        }
        return nullptr;
      }

      static Value* top() {
        return top_ ? top_->value_ : nullptr;
      }

    private:
      thread_local static context* top_;
    };

    template <typename Key, typename Value>
    thread_local call_stack<Key, Value>::context*
        call_stack<Key, Value>::top_ = nullptr;
  }  // namespace base
}  // namespace easio

#endif