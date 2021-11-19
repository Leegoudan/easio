#ifndef EASIO_BASE_WIN_IOCP_OPERATION_HPP
#define EASIO_BASE_WIN_IOCP_OPERATION_HPP
#pragma once

#include <functional>
#include <memory>
#include <system_error>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace easio {
  namespace base {

    //class execution_context::service;
    using service_ptr = std::shared_ptr<void>;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    class win_iocp_operation
      : public OVERLAPPED,
        public std::enable_shared_from_this<win_iocp_operation> {
    public:
      using operation = win_iocp_operation;
      using operation_ptr = std::shared_ptr<operation>;

      void complete(service_ptr owner, const std::error_code& ec,
                    std::size_t bytes_transferred) {
        func_(owner, shared_from_this(), ec, bytes_transferred);
      }

      void destroy() {
        func_(nullptr, shared_from_this(), std::error_code(), 0);
      }

    protected:
      using func_type = std::function<
        void(service_ptr, operation_ptr, const std::error_code&, std::size_t)>;

      win_iocp_operation(func_type func)
          : func_(func) {
        reset();
      }

      // Prevents deletion through this type.
      //~win_iocp_operation() {
      //}

      void reset() {
        Internal = 0;
        InternalHigh = 0;
        Offset = 0;
        OffsetHigh = 0;
        hEvent = 0;
        ready_ = 0;
      }

    private:
      friend class win_iocp_io_context;
      operation_ptr next_;
      func_type func_;
      long ready_;
    };

    using operation = win_iocp_operation;
    using operation_ptr = std::shared_ptr<operation>;
#else
    class scheduler_operation
      : public std::enable_shared_from_this<scheduler_operation>
    {
    public:
      using operation = scheduler_operation;
      using operation_ptr = std::shared_ptr<operation>;

      void complete(service_ptr owner, const std::error_code& ec,
          std::size_t bytes_transferred)
      {
        func_(owner, shared_from_this(), ec, bytes_transferred);
      }

      void destroy()
      {
        func_(0, shared_from_this(), std::error_code(), 0);
      }

    protected:
      using func_type = std::function<
          void(service_ptr, operation_ptr, const std::error_code&, std::size_t)>;

      scheduler_operation(func_type func)
        : func_(func),
          task_result_(0)
      {
      }

      // Prevents deletion through this type.
      //~scheduler_operation()
      //{
      //}

    private:
      friend class scheduler;
      operation_ptr next_;
      func_type func_;
      unsigned int task_result_; // Passed into bytes transferred.
    };

    using operation = scheduler_operation;
    using operation_ptr = std::shared_ptr<operation>;
#endif

    class resolve_op 
      : public operation {
    public:
      std::error_code ec_;

    protected:
      resolve_op(func_type complete_func) : operation(complete_func) {}
    };
    using resolve_op_ptr = std::shared_ptr<resolve_op>;

    class wait_op
      : public operation {
    public:
      std::error_code ec_;
    protected:
      wait_op(func_type func) : operation(func) {}
    };
    using wait_op_ptr = std::shared_ptr<wait_op>;
  
  }  // namespace base
}  // namespace easio


#endif