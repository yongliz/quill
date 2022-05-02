/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/LogLevel.h"
#include "quill/detail/misc/Common.h"
#include <cstdint>

namespace quill
{
/**
 * Captures and stores information about a logging event in compile time
 * This information is later passed to the LogEvent runtime class
 */
class MacroMetadata
{
public:
  enum Event : uint8_t
  {
    Log,
    InitBacktrace,
    FlushBacktrace,
    Flush
  };

  constexpr MacroMetadata(const char* lineno, char const* pathname, char const* func,
                          char const* message_format, LogLevel level, Event event)
    : _func(func),
      _pathname(pathname),
      _filename(_extract_source_file_name(_pathname)),
      _message_format(message_format),
      _lineno(lineno),
      _level(level),
      _event(event)
  {
  }

#if defined(_WIN32)
  constexpr MacroMetadata(const char* lineno, char const* pathname, char const* func,
                          wchar_t const* message_format, LogLevel level)
    : _func(func),
      _pathname(pathname),
      _filename(_extract_source_file_name(_pathname)),
      _lineno(lineno),
      _level(level),
      _has_wide_char(true),
      _wmessage_format(message_format)
  {
  }
#endif

  /**
   * @return The function name
   */
  QUILL_NODISCARD constexpr char const* func() const noexcept { return _func; }

  /**
   * @return The full pathname of the source file where the logging call was made.
   */
  QUILL_NODISCARD constexpr char const* pathname() const noexcept { return _pathname; }

  /**
   * @return Short portion of the path name
   */
  QUILL_NODISCARD constexpr char const* filename() const noexcept { return _filename; }

  /**
   * @return The user provided format
   */
  QUILL_NODISCARD constexpr char const* message_format() const noexcept { return _message_format; }

  /**
   * @return The line number
   */
  QUILL_NODISCARD constexpr char const* lineno() const noexcept { return _lineno; }

  /**
   * @return The log level of this logging event as an enum
   */
  QUILL_NODISCARD constexpr LogLevel level() const noexcept { return _level; }

  /**
   * @return  The log level of this logging event as a string
   */
  QUILL_NODISCARD constexpr char const* level_as_str() const noexcept
  {
    return _log_level_to_string(_level);
  }

  /**
   * @return  The log level of this logging event as a string
   */
  QUILL_NODISCARD constexpr char const* level_id_as_str() const noexcept
  {
    return _log_level_id_to_string(_level);
  }

  QUILL_NODISCARD constexpr Event event() const noexcept { return _event; }

#if defined(_WIN32)
  /**
   * @return true if the user provided a wide char format string
   */
  QUILL_NODISCARD constexpr bool has_wide_char() const noexcept { return _has_wide_char; }

  /**
   * @return The user provided wide character format
   */
  QUILL_NODISCARD constexpr wchar_t const* wmessage_format() const noexcept
  {
    return _wmessage_format;
  }
#endif

private:
  QUILL_NODISCARD static constexpr char const* _extract_source_file_name(char const* path) noexcept
  {
    const char* file = path;
    while (*path)
    {
      if (*path++ == path_delimiter)
      {
        file = path;
      }
    }
    return file;
  }

  QUILL_NODISCARD static constexpr char const* _log_level_to_string(LogLevel log_level)
  {
    constexpr std::array<char const*, 10> log_levels_strings = {
      {"TRACE_L3 ", "TRACE_L2 ", "TRACE_L1 ", "DEBUG    ", "INFO     ", "WARNING  ", "ERROR    ",
       "CRITICAL ", "BACKTRACE", "NONE"}};

    using log_lvl_t = std::underlying_type<LogLevel>::type;
    return log_levels_strings[static_cast<log_lvl_t>(log_level)];
  }

  QUILL_NODISCARD static constexpr char const* _log_level_id_to_string(LogLevel log_level)
  {
    constexpr std::array<char const*, 10> log_levels_strings = {
      {"T3", "T2", "T1", "D", "I", "W", "E", "C", "BT", "N"}};

    using log_lvl_t = std::underlying_type<LogLevel>::type;
    return log_levels_strings[static_cast<log_lvl_t>(log_level)];
  }

private:
  char const* _func{nullptr};
  char const* _pathname{nullptr};
  char const* _filename{nullptr};
  char const* _message_format{nullptr};
  char const* _lineno{nullptr};
  LogLevel _level{LogLevel::None};
  Event _event{Event::Log};

#if defined(_WIN32)
  bool _has_wide_char{false};
  wchar_t const* _wmessage_format{nullptr};
#endif
};
} // namespace quill