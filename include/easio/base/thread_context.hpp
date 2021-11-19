#ifndef EASIO_BASE_THREAD_CONTEXT_HPP
#define EASIO_BASE_THREAD_CONTEXT_HPP
#pragma once

#include <exception>
#include <limits>
#include <concepts>

#include "base/call_stack.hpp"

#if !defined(WIN32) && !defined(_WIN32) && !defined(__WIN32__)
#include <cstdlib>
#endif
inline void* aligned_new(std::size_t align, std::size_t size) {
  size = (size % align == 0) ? size : size + (align - size % align);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  void* ptr = _aligned_malloc(size, align);
#else
  void* ptr = std::aligned_alloc(align, size);
#endif
  if (!ptr) {
    throw std::bad_alloc{};
  }
  return ptr;
}

inline void aligned_delete(void* ptr) {
  #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}

namespace easio {
  namespace base {

    class multiple_exceptions : public std::exception {
    public:
      inline multiple_exceptions(std::exception_ptr first) noexcept
          : first_(std::move(first)) {}

      inline virtual const char* what() const noexcept {
        return "multiple exceptions";
      }

      inline std::exception_ptr first_exception() const {
        return first_;
      }

    private:
      std::exception_ptr first_;
    };

    static const int RECYCLING_ALLOCATOR_CACHE_SIZE = 2;
    
    template<typename T>
    concept purpose = requires {
      T::cache_size;
      T::begin_mem_index;
      T::end_mem_index;
    };

    class thread_info : private noncopyable {
    public:
      struct default_tag {
        static const int cache_size = RECYCLING_ALLOCATOR_CACHE_SIZE;
        static const int begin_mem_index = 0;
        static const int end_mem_index = begin_mem_index + cache_size;
      };

      struct awaitable_frame_tag {
        static const int cache_size = RECYCLING_ALLOCATOR_CACHE_SIZE;
        static const int begin_mem_index = default_tag::end_mem_index;
        static const int end_mem_index = begin_mem_index + cache_size;
      };

      struct executor_function_tag {
        static const int cache_size = RECYCLING_ALLOCATOR_CACHE_SIZE;
        static const int begin_mem_index = awaitable_frame_tag::end_mem_index;
        static const int end_mem_index = begin_mem_index + cache_size;
      };

      struct cancellation_signal_tag {
        static const int cache_size = RECYCLING_ALLOCATOR_CACHE_SIZE;
        static const int begin_mem_index = executor_function_tag::end_mem_index;
        static const int end_mem_index = begin_mem_index + cache_size;
      };

      struct parallel_group_tag {
        static const int cache_size = RECYCLING_ALLOCATOR_CACHE_SIZE;
        static const int begin_mem_index = cancellation_signal_tag::end_mem_index;
        static const int end_mem_index = begin_mem_index + cache_size;
      };

      static const int max_mem_index = parallel_group_tag::end_mem_index;
      static const int chunk_size = 4;

      thread_info()
          : has_pending_exception_(0) {
        for (int i = 0; i < max_mem_index; ++i)
          reusable_memory_[i] = nullptr;
      }

      ~thread_info() {
        for (int i = 0; i < max_mem_index; ++i) {
          if (reusable_memory_[i])
            aligned_delete(reusable_memory_[i]);
        }
      }

      static void* allocate(thread_info* this_thread, std::size_t size, std::size_t align = alignof(std::max_align_t)) {
        return allocate(default_tag(), this_thread, size, align);
      }

      static void deallocate(thread_info* this_thread, void* pointer, std::size_t size) {
        deallocate(default_tag(), this_thread, pointer, size);
      }

      template <purpose Purpose>
      static void* allocate(Purpose, thread_info* this_thread, std::size_t size, std::size_t align = alignof(std::max_align_t)) {
        std::size_t chunks = (size + chunk_size - 1) / chunk_size;
        if (this_thread) {
          for (int mem_index = Purpose::begin_mem_index; mem_index < Purpose::end_mem_index; ++mem_index) {
            if (this_thread->reusable_memory_[mem_index]) {
              void* const pointer = this_thread->reusable_memory_[mem_index];
              unsigned char* const mem = static_cast<unsigned char*>(pointer);
              if (static_cast<std::size_t>(mem[0]) >= chunks &&
                  reinterpret_cast<std::size_t>(pointer) % align == 0) {
                this_thread->reusable_memory_[mem_index] = 0;
                mem[size] = mem[0];
                return pointer;
              }
            }
          }

          for (int mem_index = Purpose::begin_mem_index; mem_index < Purpose::end_mem_index; ++mem_index) {
            if (this_thread->reusable_memory_[mem_index]) {
              void* const pointer = this_thread->reusable_memory_[mem_index];
              this_thread->reusable_memory_[mem_index] = 0;
              aligned_delete(pointer);
              break;
            }
          }
        }

        void* const pointer = aligned_new(align, chunks* chunk_size + 1);
        unsigned char* const mem = static_cast<unsigned char*>(pointer);
        mem[size] = (chunks <=  0xff/*std::numeric_limits<unsigned char>::max()*/) ? static_cast<unsigned char>(chunks) : 0;
        return pointer;
      }

      template <purpose Purpose>
      static void deallocate(Purpose, thread_info* this_thread, void* pointer, std::size_t size) {
        if (size <= chunk_size * 0xff/*std::numeric_limits<unsigned char>::max()*/) {
          if (this_thread) {
            for (int mem_index = Purpose::begin_mem_index; mem_index < Purpose::end_mem_index; ++mem_index) {
               if(this_thread->reusable_memory_[mem_index] == 0) {
                  unsigned char* const mem = static_cast<unsigned char*>(pointer);
                  mem[0] = mem[size];
                  this_thread->reusable_memory_[mem_index] = pointer;
                  return;
               }
            }
          }
        }

        aligned_delete(pointer);
      }

      void capture_current_exception() {
        switch (has_pending_exception_) {
        case 0:
          has_pending_exception_ = 1;
          pending_exception_ = std::current_exception();
          break;
        case 1:
          has_pending_exception_ = 2;
          pending_exception_ =
              std::make_exception_ptr<multiple_exceptions>(pending_exception_);
          break;
        default:
          break;
        }
      }

      void rethrow_pending_exception() {
        if (has_pending_exception_ > 0) {
          has_pending_exception_ = 0;
          std::exception_ptr ex(std::move(pending_exception_));
          std::rethrow_exception(ex);
        }
      }

    private:
      void* reusable_memory_[max_mem_index];
      int has_pending_exception_;
      std::exception_ptr pending_exception_;
    };

    class thread_context {
    protected:
      typedef call_stack<thread_context, thread_info> thread_call_stack;

    public:
      inline static thread_info* top_of_thread_call_stack() {
        return thread_call_stack::top();
      }
    };
  }  // namespace base
}  // namespace easio

#endif