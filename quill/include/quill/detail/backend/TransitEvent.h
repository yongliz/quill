/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/Serialize.h"
#include "quill/detail/ThreadContext.h"
#include "quill/detail/misc/Common.h"

namespace quill
{
namespace detail
{
struct TransitEvent
{
  TransitEvent(ThreadContext* thread_context, detail::Header header,
               FormatFnMemoryBuffer formatted_msg, std::atomic<bool>* flush_flag)
    : thread_context(thread_context), header(header), formatted_msg(std::move(formatted_msg)), flush_flag(flush_flag)
  {
  }

  TransitEvent(TransitEvent const& other)
    : thread_context(other.thread_context), header(other.header), flush_flag(other.flush_flag)

  {
    formatted_msg.append(other.formatted_msg.begin(), other.formatted_msg.end());
  }

  TransitEvent& operator=(TransitEvent other)
  {
    std::swap(thread_context, other.thread_context);
    std::swap(header, other.header);
    std::swap(formatted_msg, other.formatted_msg);
    std::swap(flush_flag, other.flush_flag);
    return *this;
  }

  friend bool operator>(TransitEvent const& lhs, TransitEvent const& rhs)
  {
    return lhs.header.timestamp > rhs.header.timestamp;
  }

  ThreadContext* thread_context; /** We clean any invalidated thread_context after the priority queue is empty, so this pointer can not be invalid */
  detail::Header header;
  FormatFnMemoryBuffer formatted_msg;     /** buffer for message **/
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};
} // namespace detail
} // namespace quill