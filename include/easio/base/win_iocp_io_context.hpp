#ifndef EASIO_BASE_WIN_IOCP_IO_CONTEXT_HPP
#define EASIO_BASE_WIN_IOCP_IO_CONTEXT_HPP
#pragma once

#include <thread>
#include <queue>

#include "base/execution_context.hpp"
#include "base/operation.hpp"
#include "base/thread_context.hpp"

namespace easio {
  namespace base {
    class win_iocp_io_context
        : public execution_context_service<win_iocp_io_context>,
          public thread_context {
    public:
      inline win_iocp_io_context(execution_context& ctx, int concurrency_hint = -1, bool own_thread = true)
        : execution_context_service<win_iocp_io_context>(ctx),
          outstanding_work_(0), stopped_(0), stop_event_posted_(0), shutdown_(0),
          get_queue_compl_stat_timeout_(get_complete_status_timeout()), concurrency_hint_(concurrency_hint) {
      
        iocp_.handle = ::CreateIoCompletionPort((HANDLE)-1, 0, 0, static_cast<DWORD>(concurrency_hint_ >= 0 ? concurrency_hint_ : DWORD(~0)));
        if(!iocp_.handle) {
          DWORD last_error = ::GetLastError();
          std::error_code ec(last_error, std::system_category());
          throw ec;
        }

        if (own_thread) {
          ::InterlockedIncrement(&outstanding_work_);
          thread_.reset(new std::thread([this]{std::error_code ec; run(ec);}));
        }
      }

      inline ~win_iocp_io_context() {
        if (thread_) {
          stop();
          thread_->join();
          thread_.reset();
        }
      }

      inline void shutdown() {
        ::InterlockedExchange(&shutdown_, 1);

        if(thread_) {
          stop();
          thread_->join();
          thread_.reset();
          ::InterlockedDecrement(&outstanding_work_);
        }

        while(::InterlockedExchangeAdd(&outstanding_work_, 0) > 0) {
          if (!completed_ops_.empty()) {
            while (operation_ptr op = completed_ops_.front()) {
              completed_ops_.pop();
              ::InterlockedDecrement(&outstanding_work_);
              op->destroy();
            }
          } else {
            DWORD bytes_transferred = 0;
            DWORD_PTR completion_key = 0;
            LPOVERLAPPED overlapped = 0;
            ::GetQueuedCompletionStatus(iocp_.handle, &bytes_transferred,
                &completion_key, &overlapped, get_queue_compl_stat_timeout_);
            if (overlapped) {
              ::InterlockedDecrement(&outstanding_work_);
              static_cast<operation*>(overlapped)->destroy();
            }
          }
        }
      }

      inline void init_task() {}

      inline bool register_handle(HANDLE handle, std::error_code& ec) {
        if(::CreateIoCompletionPort(handle, iocp_.handle, 0, 0) == 0) {
          DWORD last_error = ::GetLastError();
          ec = std::error_code(last_error, std::system_category());
        } else {
          ec = std::error_code();
        }

        return ec.value() != 0;
      }

      inline size_t run(std::error_code& ec) {
        if (::InterlockedExchangeAdd(&outstanding_work_, 0) == 0) {
          stop();
          ec = std::error_code();
          return 0;
        }

        thread_info this_thread;
        thread_call_stack::context ctx(this, this_thread);
        size_t n = 0;
        while (do_one(INFINITE, this_thread, ec))
          if (n != (std::numeric_limits<size_t>::max)())
            ++n;
        return n;
      }

      inline size_t run_one(std::error_code& ec) {
        if (::InterlockedExchangeAdd(&outstanding_work_, 0) == 0) {
          stop();
          ec = std::error_code();
          return 0;
        }

        thread_info this_thread;
        thread_call_stack::context ctx(this, this_thread);

        return do_one(INFINITE, this_thread, ec);
      }

      inline size_t wait_one(long usec, std::error_code& ec) {
        if (::InterlockedExchangeAdd(&outstanding_work_, 0) == 0) {
          stop();
          ec = std::error_code();
          return 0;
        }

        thread_info this_thread;
        thread_call_stack::context ctx(this, this_thread);

        return do_one(usec < 0 ? INFINITE : ((usec - 1) / 1000 + 1), this_thread, ec);
      }

      inline void stop() {
        if (::InterlockedExchange(&stopped_, 1) == 0 &&
            ::InterlockedExchange(&stop_event_posted_, 1) == 0 &&
            !::PostQueuedCompletionStatus(iocp_.handle, 0, 0, 0)) {
          DWORD last_error = ::GetLastError();
          std::error_code ec(last_error, std::system_category());
          throw ec;
        }
      }

      bool stopped() const { return ::InterlockedExchangeAdd(&stopped_, 0) != 0; }

      void restart() { ::InterlockedExchange(&stopped_, 0); }

      void work_started() { ::InterlockedIncrement(&outstanding_work_); }

      void work_finished() {
        if (::InterlockedDecrement(&outstanding_work_) == 0)
          stop();
      }

      inline bool can_dispatch() { return thread_call_stack::contains(this) != 0; }

      inline void capture_current_exception() {
        if (thread_info* this_thread = thread_call_stack::contains(this))
          this_thread->capture_current_exception();
      }

      void post_immediate_completion(operation_ptr op, bool) {
        work_started();
        post_deferred_completion(op);
      }

      inline void post_deferred_completion(operation_ptr op) {
        op->ready_ = 1;

        if (!::PostQueuedCompletionStatus(iocp_.handle, 0, 0, op.get())) {
          std::lock_guard<std::mutex> lock(dispatch_mutex_);
          completed_ops_.push(op);
          ::InterlockedExchange(&dispatch_required_, 1);
        }
      }

      inline void post_deferred_completions(std::queue<operation_ptr>& ops) {
        while (operation_ptr op = ops.front()) {
          ops.pop();
          op->ready_ = 1;

          if (!::PostQueuedCompletionStatus(iocp_.handle, 0, 0, op.get())) {
              std::lock_guard<std::mutex> lock(dispatch_mutex_);
              completed_ops_.push(op);
              completed_ops_.push(ops);
              ::InterlockedExchange(&dispatch_required_, 1);
          }
        }
      }

      void post_private_immediate_completion(operation_ptr op) {
        post_immediate_completion(op, false);
      }

      void post_private_deferred_completion(operation_ptr op) {
        post_deferred_completion(op);
      }

      void do_dispatch(operation_ptr op) {
        post_immediate_completion(op, false);
      }

      inline void abandon_operations(std::queue<operation_ptr>& ops) {
        while (operation_ptr op = ops.front()) {
          ops.pop();
          ::InterlockedDecrement(&outstanding_work_);
          op->destroy();
        }
      }

      inline void on_pending(operation_ptr op) {
        if (::InterlockedCompareExchange(&op->ready_, 1, 0) == 1) {
          if (!::PostQueuedCompletionStatus(iocp_.handle, 0, overlapped_contains_result, op.get())) {
            std::lock_guard<std::mutex> lock(dispatch_mutex_);
            completed_ops_.push(op);
            ::InterlockedExchange(&dispatch_required_, 1);
          }
        }
      }

      inline void on_completion(operation_ptr op, DWORD last_error = 0, DWORD bytes_transferred = 0) {
        op->ready_ = 1;

        op->Internal = reinterpret_cast<ulong_ptr_t>(&std::system_category());
        op->Offset = last_error;
        op->OffsetHigh = bytes_transferred;

        if (!::PostQueuedCompletionStatus(iocp_.handle, 0, overlapped_contains_result, op.get())) {
          std::lock_guard<std::mutex> lock(dispatch_mutex_);
          completed_ops_.push(op);
          ::InterlockedExchange(&dispatch_required_, 1);
        }
      }

      inline void on_completion(operation_ptr op, const std::error_code& ec, DWORD bytes_transferred = 0) {
        op->ready_ = 1;

        op->Internal = reinterpret_cast<ulong_ptr_t>(&ec.category());
        op->Offset = ec.value();
        op->OffsetHigh = bytes_transferred;

        if (!::PostQueuedCompletionStatus(iocp_.handle, 0, overlapped_contains_result, op.get())) {
          std::lock_guard<std::mutex> lock(dispatch_mutex_);
          completed_ops_.push(op);
          ::InterlockedExchange(&dispatch_required_, 1);
        }
      }

      int concurrency_hint() const { return concurrency_hint_; }

    private:

      inline size_t do_one(DWORD msec, thread_info& this_thread, std::error_code& ec) {
        while(true) {
          if (::InterlockedCompareExchange(&dispatch_required_, 0, 1) == 1) {
            std::lock_guard<std::mutex> lock(dispatch_mutex_);

            post_deferred_completions(completed_ops_);
          }

          DWORD bytes_transferred = 0;
          dword_ptr_t completion_key = 0;
          LPOVERLAPPED overlapped = 0;
          ::SetLastError(0);
          bool ok = ::GetQueuedCompletionStatus(iocp_.handle,
              &bytes_transferred, &completion_key, &overlapped,
              msec < gqcs_timeout_ ? msec : gqcs_timeout_);
          DWORD last_error = ::GetLastError();
          
          if (overlapped) {
            operation* op = static_cast<operation*>(overlapped);
            std::error_code result_ec(last_error, std::system_category());
            
            if (completion_key == overlapped_contains_result) {
              result_ec = std::error_code(static_cast<int>(op->Offset), *reinterpret_cast<std::error_category*>(op->Internal));
              bytes_transferred = op->OffsetHigh;
            } else {
              op->Internal = reinterpret_cast<ulong_ptr_t>(&result_ec.category());
              op->Offset = result_ec.value();
              op->OffsetHigh = bytes_transferred;
            }
        }
      }

      inline static DWORD get_complete_status_timeout();

      inline void update_timeout();

      struct work_finished_on_block_exit {
        ~work_finished_on_block_exit() { _c->work_finished(); }

        win_iocp_io_context* _c;
      };

      struct auto_handle {
        HANDLE handle;
        auto_handle() : handle(0) {}
        ~auto_handle() {
          if (handle)
            ::CloseHandle(handle);
        }
      } iocp_;

      long outstanding_work_;

      mutable long stopped_;

      long stop_event_posted_;

      long shutdown_;

      static const int default_gqcs_timeout = 500;
      static const int max_timeout_msec = 5 * 60 * 1000;
      static const int max_timeout_usec = max_timeout_msec * 1000;
      static const int wake_for_dispatch = 1;
      static const int overlapped_contains_result = 2;

      const DWORD get_queue_compl_stat_timeout_;

      long dispatch_required_;
      std::mutex dispatch_mutex_;
      
      std::queue<operation_ptr> completed_ops_;
      const int concurrency_hint_;
      std::unique_ptr<std::thread> thread_;
      
    };
  }
}  // namespace easio

#include "base/impl/win_iocp_io_context.ipp"

#endif