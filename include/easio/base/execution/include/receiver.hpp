//
// Copyright (c) 2021- Lee Goudan
// Last Modified: 2021-08-30
// Distributed under The MIT License (MIT) at https://mit-license.org/
//

#ifndef EASIO_BASE_EXECUTION_RECEIVER_HPP
#define EASIO_BASE_EXECUTION_RECEIVER_HPP
#pragma once

#include <exception>
#include "include/tag_invoke.hpp"

namespace easio {
  namespace base {
    namespace execution {
      inline namespace _receiver_cpo {
        /////////////////////////////////////////////////////////////////////////////
        // [execution.receivers.set_value]
        inline constexpr struct set_value_t {
          template <typename R, typename... Values>
          requires tag_invocable<set_value_t, R, Values...>
          void operator()(R&& r, Values&&... vs) const
              noexcept(nothrow_tag_invocable<set_value_t, R, Values...>) {
            (void)tag_invoke(set_value_t{}, std::forward(R), std::forward(vs));
          }
        } set_value{};
        /////////////////////////////////////////////////////////////////////////////
        // [execution.receivers.set_error]
        inline constexpr struct set_error_t {
          template <typename R, typename E = std::exception_ptr>
          requires tag_invocable<set_error_t, R, E>
          void operator()(R&& r, E&& e) const
              noexcept(nothrow_tag_invocable<set_error_t, R, E>) {
            (void)tag_invoke(set_error_t{}, std::forward(R), std::forward(e));
          }
        } set_error{};
        /////////////////////////////////////////////////////////////////////////////
        // [execution.receivers.set_done]
        inline constexpr struct set_done_t {
          template <typename R>
          requires tag_invocable<set_done_t, R>
          void operator()(R&& r) const
              noexcept(nothrow_tag_invocable<set_done_t, R>) {
            (void)tag_invoke(set_done_t{}, std::forward(R));
          }
        } set_done{};
      }  // namespace _receiver_cpo
      /////////////////////////////////////////////////////////////////////////////
      // [execution.receivers]
      template <typename R, typename E = std::exception_ptr>
      concept receiver =
          std::move_constructible<std::remove_cvref_t<R>> &&
          std::constructible_from<std::remove_cvref_t<R>> &&
          requires(std::remove_cvref_t<R>&& r, E&& e) {
        { set_done(std::move(r)) }
        noexcept;
        { set_error(std::move(r), std::forward(e)) }
        noexcept;
      };

      template <typename R, typename... Values>
      concept receiver_of = receiver<R> &&
          requires(std::remove_cvref_t<R>&& r, Values... vs) {
        {set_value(std::move(r), std::forward(vs...))};
      };

      template <typename R, typename... Values>
      concept nothrow_receiver_of = receiver_of<R, Values...> &&
          nothrow_tag_invocable<set_value_t, R, Values...>;

    };  // namespace execution
  }     // namespace base
}  // namespace easio

#endif