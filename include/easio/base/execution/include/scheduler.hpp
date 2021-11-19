//
// Copyright (c) 2021- Lee Goudan
// Last Modified: 2021-09-02
// Distributed under The MIT License (MIT) at https://mit-license.org/
//

#ifndef EASIO_BASE_EXECUTION_SCHEDULER_HPP
#define EASIO_BASE_EXECUTION_SCHEDULER_HPP
#pragma once

#include "include/sender.hpp"

namespace easio {
  namespace base {
    namespace execution {
      /////////////////////////////////////////////////////////////////////////////
      // [execution.schedulers]
      template<typename S>
      concept scheduler = 
        std::copy_constructible<std::remove_cvref_t<S>> &&
        std::equality_comparable<std::remove_cvref_t<S>> &&
        requires(S&& s) { schedule(std::forward(s)); };
      /////////////////////////////////////////////////////////////////////////////
      // [execution.schedulers.queries]
      enum class forward_progress_guarantee {
        concurrent,
        parallel,
        weakly_parallel
      };

      inline namespace _get_forward_progress_guarantee_cpo {
        inline constexpr struct get_forward_progress_guarantee_t {
          
        } get_forward_progress_guarantee{};
      }
    }
  }
}

#endif