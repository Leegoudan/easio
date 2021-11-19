//
// Copyright (c) 2021- Lee Goudan
// Last Modified: 2021-09-01
// Distributed under The MIT License (MIT) at https://mit-license.org/
//

#ifndef EASIO_BASE_EXECUTION_OP_STATE_HPP
#define EASIO_BASE_EXECUTION_OP_STATE_HPP
#pragma once

#include "include/tag_invoke.hpp"

namespace easio {
  namespace base {
    namespace execution {
      /////////////////////////////////////////////////////////////////////////////
      // [execution.op_state.start]

      inline namespace _start_cpo {
        inline constexpr struct start_t {
          template <typename Op>
          requires tag_invocable<start_t, Op&>
          void operator()(Op& o) const noexcept(nothrow_tag_invocable<start_t, Op&>) {
            (void)tag_invoke(start_t{}, o);
          }
        } start{};
      }  // namespace _start_cpo

      /////////////////////////////////////////////////////////////////////////////
      // [execution.op_state]
      template <typename Op>
      concept operation_state = std::destructible<Op> &&
                                std::is_object_v<Op> &&
                                requires(Op& o) { {start(o)} noexcept; };
    }  // namespace execution
  }    // namespace base
}  // namespace easio

#endif