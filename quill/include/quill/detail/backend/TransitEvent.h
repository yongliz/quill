#pragma once

#include "quill/LogMacroMetadata.h"
#include "quill/detail/Header.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/ThreadContext.h"

namespace quill
{
namespace detail
{
struct TransitEvent
{
  TransitEvent(ThreadContext* thread_context, detail::Header header, MemoryBuffer formatted_msg,
               std::atomic<bool>* flush_flag)
    : thread_context(thread_context), header(header), formatted_msg(std::move(formatted_msg)), flush_flag(flush_flag)
  {
  }

  TransitEvent(TransitEvent const& other)
    : thread_context(other.thread_context), header(other.header)

  {
    formatted_msg.append(other.formatted_msg.begin(), other.formatted_msg.end());
  }

  TransitEvent& operator=(TransitEvent other)
  {
    std::swap(thread_context, other.thread_context);
    std::swap(header, other.header);
    std::swap(formatted_msg, other.formatted_msg);
    return *this;
  }

  friend bool operator>(TransitEvent const& lhs, TransitEvent const& rhs)
  {
    return lhs.header.timestamp > rhs.header.timestamp;
  }

  ThreadContext* thread_context; /** We clean any invalidated thread_context after the priority queue is empty, so this pointer can not be invalid */

  detail::Header header;
  MemoryBuffer formatted_msg;             /** buffer for message **/
  std::atomic<bool>* flush_flag{nullptr}; /** This is only used in the case of Event::Flush **/
};
} // namespace detail
} // namespace quill