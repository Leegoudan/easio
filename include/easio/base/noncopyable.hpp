#ifndef EASIO_BASE_NONCOPYABLE_HPP
#define EASIO_BASE_NONCOPYABLE_HPP
#pragma once

namespace easio
{
  namespace base
  {
    class noncopyable
    {
    protected:
      noncopyable() = default;
      ~noncopyable() = default;
      noncopyable(const noncopyable &) = delete;
      const noncopyable &operator=(const noncopyable &) = delete;
    };
  } // base
  using easio::base::noncopyable;
} // easio
#endif