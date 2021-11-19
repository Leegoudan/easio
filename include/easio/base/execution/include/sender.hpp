//
// Copyright (c) 2021- Lee Goudan
// Last Modified: 2021-08-25
// Distributed under The MIT License (MIT) at https://mit-license.org/
//

#ifndef EASIO_BASE_EXECUTION_SENDER_HPP
#define EASIO_BASE_EXECUTION_SENDER_HPP
#pragma once

#include <iostream>

#include "include/op_state.hpp"
#include "include/receiver.hpp"

namespace easio {
  namespace base {
    namespace execution {
      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders.traits]
      struct sender_base {};

      template <typename>
      struct sender_trait_base {
        using __unspecialized = void;
      };

      template <typename S>
      requires(has_sender_types<S>) struct sender_trait_base<S> {
        template <template <typename...> typename Tuple, template <typename...> typename Variant>
        using value_types = typename S::template value_types<Tuple, Variant>;

        template <template <typename...> typename Variant>
        using error_types = typename S::template error_types<Variant>;

        static constexpr bool sends_done = S::sends_done;
      };

      template <typename S>
      requires(!has_sender_types<S> && std::derived_from<S, sender_base>) struct sender_trait_base<S> {
      };

      template <typename S>
      struct sender_traits : sender_trait_base<S> {};
      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders]
      template <typename S>
      concept sender =
          std::move_constructible<std::remove_cvref_t<S>> &&
          !requires {
            typename sender_traits<std::remove_cvref_t<S>>::__unspecialized;
          };

      template <template <template <typename...> typename, template <typename...> typename> typename>
      struct _has_value_types;

      template <template <template <typename...> typename> typename>
      struct _has_error_types;

      template <typename S>
      concept has_sender_types = requires {
        typename _has_value_types<S::template value_types>;
        typename _has_error_types<S::template error_types>;
        typename std::bool_constant<S::send_done>;
      };

      template <typename S>
      concept typed_sender = sender<S> &&
          has_sender_types<sender_traits<std::remove_cvref_t<S>>>;

      template <typename... Types>
      struct type_list {};

      template <typename S, typename... Types>
      concept sender_of = typed_sender<S> &&
                        std::same_as <
                        type_list<Types...>,
      typename sender_traits<S>::template value_types<type_list, std::type_identity_t> > ;
      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders.connect]
      inline namespace _connect_cpo {
        inline constexpr struct connect_t {
          template <sender S, receiver R>
          requires tag_invocable<connect_t, S, R> &&
              operation_state<tag_invoke_result_t<connect_t, S, R>>
          auto operator()(S&& s, R&& r) const
              noexcept(nothrow_tag_invocable<connect_t, S, R>) {
            return tag_invoke(connect_t{}, std::forward(s), std::forward(r));
          }
        } connect{};
      }  // namespace _connect_cpo
      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders]
      template <typename S, typename R>
      concept sender_to =
          sender<S> && receiver<R> &&
          requires(S&& s, R&& r) {
        connect(std::forward(s), std::forward(r));
      };
      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders.queries], sender queries
      inline namespace _get_completion_scheduler_cpo {
        inline constexpr struct get_completion_scheduler_t {
          template <typename CPO, sender S>
          requires tag_invocable<get_completion_scheduler_t, CPO, S>
          auto operator()(S&& s) const
              noexcept(nothrow_tag_invocable<get_completion_scheduler_t, CPO, S>) {
            return tag_invoke(get_completion_scheduler_t{}, std::forward(s));
          }

        } get_completion_scheduler{};
      }  // namespace _get_completion_scheduler_cpo

      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders.schedule]
      inline namespace _schedule_cpo {
        inline constexpr struct schedule_t {
          template <typename S>
          requires tag_invocable<schedule_t, S> &&
              sender<tag_invoke_result_t<schedule_t, S>>
          auto operator()(S&& s) const
              noexcept(nothrow_tag_invocable<schedule_t, S>) {
            return tag_invoke(schedule_t{}, std::forward(s));
          }
        } schedule{};
      }  // namespace _schedule_cpo
      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders.consumer.start_detached]
      inline namespace _consumer_start_detached_cpo {
        inline constexpr struct consumer_start_detached_t {
          template <receiver R, sender_to<R> S>
          requires tag_invocable<consumer_start_detached_t, S, R>
          void operator()(S&& s, R&& r) const
              noexcept(nothrow_tag_invocable<consumer_start_detached_t, S, R>) {
            (void)tag_invoke(consumer_start_detached_t{}, std::forward(s), std::forward(r));
          }
        } consumer_start_detached{};
      }  // namespace _consumer_start_detached_cpo

      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders.just]
      inline namespace _just_cpo {
        template <typename CPO, typename... Ts>
        struct _just_sender_traits {
        };

        template <typename... Ts>
        struct _just_sender {
          std::tuple<Ts...> vs_;
          template <template <typename...> typename Tuple, template <typename...> typename Variant>
          using value_types = Variant<Tuple<Ts...>>;

          template <template <typename...> typename Variant>
          using error_types = Variant<>;

          static constexpr bool sends_done = false;

          template <typename R>
          struct operation_state {
            std::tuple<Ts...> vs_;
            R r_;

            void tag_invoke(start_t)
              noexcept(noexcept(set_value(std::declval<R>(), std::declval<Ts>()...))) {
              try {
                std::apply(
                    [&](Ts&&... values_) { set_value(std::move(r_), std::forward(values_)...); },
                    std::move(vs_));
              } catch (...) {
                set_error(std::move(r_), std::current_exception());
              }
            }
          };

          template <receiver R>
          requires receiver_of<R, Ts...> &&(std::copyable<Ts>&&...) auto tag_invoke(connect_t, R&& r) const& {
            return operation_state<R>(vs_, std::forward(r));
          }

          template <receiver R>
          requires receiver_of<R, Ts...>
          auto tag_invoke(connect_t, R&& r) && {
            return operation_state<R>(std::move(vs_), std::forward(r));
          }
        };

        inline constexpr struct just_t {
          template <typename... Ts>
            requires(std::constructible_from<std::decay_t<Ts>, Ts>&&...)
          _just_sender<set_value_t, std::decay_t<Ts>...> operator()(Ts&&... ts) const
            noexcept((std::is_nothrow_constructible_v<std::decay_t<Ts>, Ts> && ...)) {
            return {{}, {std::forward(ts)...}};
          }
        } just{};

        inline constexpr struct just_error_t {
          template<typename Error>
            requires std::constructible_from<std::decay_t<Error>, Error>
          _just_sender<set_error_t, std::decay_t<Error>> operator()(Error&& e) const
            noexcept(std::is_nothrow_constructible_v(std::decay_t<Error>, Error)) {
            return {{}, {std::forward(e)}};
          }
        } just_error{};

        inline constexpr struct just_done_t {
          _just_sender<set_done_t> operator()() const noexcept {
            return {{}};
          }
        } just_done{};
      }  // namespace _just_cpo
      
      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders.transfer_just]
      inline namespace _transfer_just_cpo {

      }
      /////////////////////////////////////////////////////////////////////////////
      // [execution.senders.adaptors]
    }    // namespace execution
  }      // namespace base
}  // namespace easio

#endif