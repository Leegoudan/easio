#ifndef EASIO_IO_CONTEXT_HPP
#define EASIO_IO_CONTEXT_HPP
#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  //using io_context_impl = win_iocp_io_context;
  //class win_iocp_overlapped_ptr;
#else
  using io_context_impl = scheduler;
#endif
#endif