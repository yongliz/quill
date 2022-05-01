/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/LogLevel.h"
#include "quill/QuillError.h"
#include "quill/detail/Header.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/ThreadContext.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/Rdtsc.h"
#include "quill/detail/misc/TypeTraitsCopyable.h"
#include "quill/detail/misc/Utilities.h"
#include <atomic>
#include <cstdint>
#include <vector>

namespace quill
{

namespace detail
{
class LoggerCollection;
}

/**
 * Thread safe logger.
 * Logger must be obtained from LoggerCollection get_logger(), therefore constructors are private
 */
class alignas(detail::CACHELINE_SIZE) Logger
{
public:
  /**
   * Deleted
   */
  Logger(Logger const&) = delete;
  Logger& operator=(Logger const&) = delete;

  /**
   * We align the logger object to it's own cache line. It shouldn't make much difference as the
   * logger object size is exactly 1 cache line
   */
  void* operator new(size_t i) { return detail::aligned_alloc(detail::CACHELINE_SIZE, i); }
  void operator delete(void* p) { detail::aligned_free(p); }

  /**
   * @return The log level of the logger
   */
  QUILL_NODISCARD LogLevel log_level() const noexcept
  {
    return _log_level.load(std::memory_order_relaxed);
  }

  /**
   * Set the log level of the logger
   * @param log_level The new log level
   */
  void set_log_level(LogLevel log_level)
  {
    if (QUILL_UNLIKELY(log_level == LogLevel::Backtrace))
    {
      QUILL_THROW(QuillError{"LogLevel::Backtrace is only used internally. Please don't use it."});
    }

    _log_level.store(log_level, std::memory_order_relaxed);
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @param log_statement_level The log level of the log statement to be logged
   * @return bool if a log record can be logged based on the current log level
   */
  QUILL_NODISCARD bool should_log(LogLevel log_statement_level) const noexcept
  {
    return log_statement_level >= log_level();
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @tparam log_statement_level The log level of the log statement to be logged
   * @return bool if a log record can be logged based on the current log level
   */
  template <LogLevel log_statement_level>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT bool should_log() const noexcept
  {
    return log_statement_level >= log_level();
  }

  /**
   * Push a log record event to the spsc queue to be logged by the backend thread.
   * One spsc queue per caller thread. This function is enabled only when all arguments are
   * fundamental types.
   * This is the fastest way possible to log
   * @note This function is thread-safe.
   * @param fmt_args format arguments
   */
  template <typename TLogMacroMetadata, typename TFormatString, typename... FmtArgs>
  QUILL_ALWAYS_INLINE_HOT void log(TFormatString format_string, FmtArgs&&... fmt_args)
  {
    fmt::detail::check_format_string<std::remove_reference_t<FmtArgs>...>(format_string);

    detail::ThreadContext* const thread_context = _thread_context_collection.local_thread_context();

    constexpr size_t c_string_count = fmt::detail::count<detail::is_type_of_c_string<FmtArgs>()...>();
    size_t c_string_sizes[std::max(c_string_count, static_cast<size_t>(1))];

    uint32_t total_size = sizeof(detail::Header) +
      static_cast<uint32_t>(detail::get_args_sizes<0>(c_string_sizes, fmt_args...));

    // request this size from the queue
    std::byte* write_buffer = thread_context->spsc_queue().prepare_write(total_size);

#if defined(QUILL_USE_BOUNDED_QUEUE)
    if (QUILL_UNLIKELY(write_buffer == nullptr))
    {
      // not enough space to push to queue message is dropped
      thread_context->increment_dropped_message_counter();
      return;
    }
#endif

    // we have enough space in this buffer, and we will write to the buffer

    // Then write the pointer to the LogDataNode. The LogDataNode has all details on how to
    // deserialize the object. We will just serialize the arguments in our queue but we need to
    // look up their types to deserialize them

    // Note: The metadata variable here is created during program init time,
    // in runtime we just get it's pointer
    std::byte* const write_begin = write_buffer;
    write_buffer = detail::align_pointer<alignof(detail::Header), std::byte>(write_buffer);

    new (write_buffer)
      detail::Header(get_metadata_ptr<TLogMacroMetadata, FmtArgs...>, std::addressof(_logger_details));
    write_buffer += sizeof(detail::Header);

    // encode remaining arguments
    write_buffer = detail::encode_args<0>(c_string_sizes, write_buffer, std::forward<FmtArgs>(fmt_args)...);

    thread_context->spsc_queue().commit_write(write_buffer - write_begin);
  }

  /**
   * Init a backtrace for this logger.
   * Stores messages logged with LOG_BACKTRACE in a ring buffer messages and displays them later on demand.
   * @param capacity The max number of messages to store in the backtrace
   * @param backtrace_flush_level If this loggers logs any message higher or equal to this severity level the backtrace will also get flushed.
   * Default level is None meaning the user has to call flush_backtrace explicitly
   */
  void init_backtrace(uint32_t capacity, LogLevel backtrace_flush_level = LogLevel::None)
  {
    // we do not care about the other fields, except quill::LogMacroMetadata::Event::InitBacktrace
    struct
    {
      constexpr quill::LogMacroMetadata operator()() const noexcept
      {
        return quill::LogMacroMetadata{
          QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__, "{}", LogLevel::Critical, quill::LogMacroMetadata::Event::InitBacktrace};
      }
    } anonymous_log_record_info;

    // we pass this message to the queue and also pass capacity as arg
    this->template log<decltype(anonymous_log_record_info)>(FMT_STRING("{}"), capacity);

    // Also store the desired flush log level
    _logger_details.set_backtrace_flush_level(backtrace_flush_level);
  }

  /**
   * Dump any stored backtrace messages
   */
  void flush_backtrace()
  {
    // we do not care about the other fields, except quill::LogMacroMetadata::Event::Flush
    struct
    {
      constexpr quill::LogMacroMetadata operator()() const noexcept
      {
        return quill::LogMacroMetadata{
          QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__, "", LogLevel::Critical, quill::LogMacroMetadata::Event::FlushBacktrace};
      }
    } anonymous_log_record_info;

    // we pass this message to the queue and also pass capacity as arg
    this->template log<decltype(anonymous_log_record_info)>(FMT_STRING(""));
  }

private:
  friend class detail::LoggerCollection;

  /**
   * Constructs new logger object
   * @param name the name of the logger
   * @param handler handlers for this logger
   * @param thread_context_collection thread context collection reference
   */
  Logger(char const* name, Handler* handler, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(name, handler), _thread_context_collection(thread_context_collection)
  {
  }

  /**
   * Constructs a new logger object with multiple handlers
   */
  Logger(char const* name, std::vector<Handler*> handlers, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(name, std::move(handlers)), _thread_context_collection(thread_context_collection)
  {
  }

private:
  detail::LoggerDetails _logger_details;
  detail::ThreadContextCollection& _thread_context_collection;
  std::atomic<LogLevel> _log_level{LogLevel::Info};
};

#if !(defined(_WIN32) && defined(_DEBUG))
// In MSVC debug mode the class has increased size
static_assert(sizeof(Logger) <= detail::CACHELINE_SIZE, "Logger needs to fit in 1 cache line");
#endif

} // namespace quill
