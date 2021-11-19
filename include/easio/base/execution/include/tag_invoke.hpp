//
// Copyright (c) 2021- Lee Goudan
// Last Modified: 2021-08-25
// Distributed under The MIT License (MIT) at https://mit-license.org/
//

#ifndef EASIO_BASE_EXECUTION_TAG_INVOKE_HPP
#define EASIO_BASE_EXECUTION_TAG_INVOKE_HPP
#pragma once

#include <concepts>

namespace easio {
  namespace base {
    namespace execution {
      inline namespace _tag_invoke {
        namespace impl {
          void tag_invoke();

          template <typename Tag, typename... Args>
          concept has_tag_invoke = requires(Tag tag, Args&&... args) {
            tag_invoke(std::forward(tag), std::forward(args...));
          };

          struct tag_invoke_t {
            template <typename... Args, has_tag_invoke<Args...> Tag>
            decltype(auto) operator()(Tag tag, Args&&... args) const
                noexcept(noexcept(tag_invoke(std::forward(tag), std::forward(args...)))) {
              return tag_invoke(std::forward(tag), std::forward(args...));
            }
          };
        }  // namespace impl
        inline constexpr struct tag_invoke_t : impl::tag_invoke_t {
        } tag_invoke{};
      }  // namespace _tag_invoke

      template <auto& Tag>
      using tag_t = std::decay_t<decltype(Tag)>;

      template <typename Tag, typename... Args>
      concept tag_invocable = std::invocable<decltype(tag_invoke), Tag, Args...>;

      template <typename Tag, typename... Args>
      concept nothrow_tag_invocable = tag_invocable<Tag, Args...> && std::is_nothrow_invocable_v<decltype(tag_invoke), Tag, Args...>;

      template <typename Tag, typename... Args>
      using tag_invoke_result = std::invoke_result<decltype(tag_invoke), Tag, Args...>;

      template <typename Tag, typename... Args>
      using tag_invoke_result_t = std::invoke_result_t<decltype(tag_invoke), Tag, Args...>;
    }  // namespace execution
  }    // namespace base
}  // namespace easio

#endif