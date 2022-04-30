/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/LogLevel.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Rdtsc.h"
#include "quill/detail/misc/Utilities.h"
#include <array>
#include <chrono>
#include <cstdint>

namespace quill
{

/**
 * Captures and stores information about a logging event in compile time
 * This information is later passed to the LogEvent runtime class
 */
class LogMacroMetadata
{
public:
  enum Event : uint8_t
  {
    Log,
    InitBacktrace,
    FlushBacktrace,
    Flush
  };

  constexpr LogMacroMetadata(const char* lineno, char const* pathname, char const* func,
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
  constexpr LogMacroMetadata(const char* lineno, char const* pathname, char const* func,
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
  QUILL_NODISCARD static constexpr char const* _str_end(char const* str) noexcept
  {
    return *str ? _str_end(str + 1) : str;
  }

  QUILL_NODISCARD static constexpr bool _str_slant(char const* str) noexcept
  {
    return *str == path_delimiter ? true : (*str ? _str_slant(str + 1) : false);
  }

  QUILL_NODISCARD static constexpr char const* _r_slant(char const* const str_begin, char const* str) noexcept
  {
    // clang-format off
    return str != str_begin ? (*str == path_delimiter ? (str + 1)
                                                      : _r_slant( str_begin, str -1))
                            : str;
    // clang-format on
  }

  QUILL_NODISCARD static constexpr char const* _extract_source_file_name(char const* str) noexcept
  {
    return _str_slant(str) ? _r_slant(str, _str_end(str)) : str;
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

// using Context = ;
using MemoryBuffer = fmt::basic_memory_buffer<char, 1024>;

namespace detail
{
template <typename Arg>
constexpr bool is_type_of_c_string()
{
  return fmt::detail::mapped_type_constant<Arg, fmt::format_context>::value == fmt::detail::type::cstring_type;
}

template <typename Arg>
constexpr bool is_type_of_string()
{
  return fmt::detail::mapped_type_constant<Arg, fmt::format_context>::value == fmt::detail::type::string_type;
}

template <typename Arg>
static inline constexpr bool need_call_dtor_for()
{
  using ArgType = fmt::remove_cvref_t<Arg>;
  if constexpr (is_type_of_string<Arg>())
  {
    return false;
  }
  return !std::is_trivially_destructible<ArgType>::value;
}

template <bool ValueOnly, size_t Idx, size_t DestructIdx>
static inline const std::byte* decodeArgs(const std::byte* in, fmt::basic_format_arg<fmt::format_context>* args,
                                          const std::byte** destruct_args)
{
  return in;
}

template <bool ValueOnly, size_t Idx, size_t DestructIdx, typename Arg, typename... Args>
static inline const std::byte* decodeArgs(const std::byte* in, fmt::basic_format_arg<fmt::format_context>* args,
                                          const std::byte** destruct_args)
{
  using ArgType = fmt::remove_cvref_t<Arg>;

  in = detail::align_pointer<alignof(Arg), std::byte const>(in);

  if constexpr (is_type_of_c_string<Arg>() || is_type_of_string<Arg>())
  {
    size_t size = strlen(reinterpret_cast<char const*>(in));
    fmt::string_view v(reinterpret_cast<char const*>(in), size);

    if constexpr (ValueOnly)
    {
      fmt::detail::value<fmt::format_context>& value_ =
        *(fmt::detail::value<fmt::format_context>*)(args + Idx);

      value_ = fmt::detail::arg_mapper<fmt::format_context>().map(v);
    }
    else
    {
      args[Idx] = fmt::detail::make_arg<fmt::format_context>(v);
    }
    return decodeArgs<ValueOnly, Idx + 1, DestructIdx, Args...>(in + size + 1, args, destruct_args);
  }
  else
  {
    if constexpr (ValueOnly)
    {
      fmt::detail::value<fmt::format_context>& value_ =
        *(fmt::detail::value<fmt::format_context>*)(args + Idx);

      value_ = fmt::detail::arg_mapper<fmt::format_context>().map(*(ArgType*)in);
    }
    else
    {
      args[Idx] = fmt::detail::make_arg<fmt::format_context>(*(ArgType*)in);
    }

    if constexpr (need_call_dtor_for<Arg>())
    {
      destruct_args[DestructIdx] = in;
      return decodeArgs<ValueOnly, Idx + 1, DestructIdx + 1, Args...>(in + sizeof(ArgType), args, destruct_args);
    }
    else
    {
      return decodeArgs<ValueOnly, Idx + 1, DestructIdx, Args...>(in + sizeof(ArgType), args, destruct_args);
    }
  }
}

template <size_t DestructIdx>
static inline void destructArgs(const std::byte**)
{
}

template <size_t DestructIdx, typename Arg, typename... Args>
static inline void destructArgs(const std::byte** destruct_args)
{
  using ArgType = fmt::remove_cvref_t<Arg>;
  if constexpr (need_call_dtor_for<Arg>())
  {
    ((ArgType*)destruct_args[DestructIdx])->~ArgType();
    destructArgs<DestructIdx + 1, Args...>(destruct_args);
  }
  else
  {
    destructArgs<DestructIdx, Args...>(destruct_args);
  }
}

typedef std::byte const* (*FormatToFn)(fmt::string_view format, const std::byte* data, MemoryBuffer& out,
                                       std::vector<fmt::basic_format_arg<fmt::format_context>>& args);

template <typename... Args>
static const std::byte* format_to(fmt::string_view format, const std::byte* data, MemoryBuffer& out,
                                  std::vector<fmt::basic_format_arg<fmt::format_context>>& args)
{
  constexpr size_t num_args = sizeof...(Args);
  constexpr size_t num_dtors = fmt::detail::count<need_call_dtor_for<Args>()...>();
  const std::byte* dtor_args[std::max(num_dtors, (size_t)1)];
  const std::byte* ret;

  auto argIdx = (int)args.size();
  args.resize(argIdx + num_args);
  ret = decodeArgs<false, 0, 0, Args...>(data, args.data() + argIdx, dtor_args);

  vformat_to(out, format, fmt::basic_format_args(args.data() + argIdx, num_args));
  destructArgs<0, Args...>(dtor_args);

  return ret;
}

template <size_t CstringIdx>
static inline constexpr size_t get_args_sizes(size_t*)
{
  return 0;
}

/**
 * Get the size of all arguments
 * @tparam CstringIdx
 * @tparam Arg
 * @tparam Args
 * @param cstringSize
 * @param arg
 * @param args
 * @return
 */
template <size_t CstringIdx, typename Arg, typename... Args>
static inline constexpr size_t get_args_sizes(size_t* c_string_sizes, const Arg& arg, const Args&... args)
{
  if constexpr (is_type_of_c_string<Arg>())
  {
    size_t const len = strlen(arg) + 1;
    c_string_sizes[CstringIdx] = len;
    return len + get_args_sizes<CstringIdx + 1>(c_string_sizes, args...);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    return (arg.size() + 1) + get_args_sizes<CstringIdx>(c_string_sizes, args...);
  }
  else
  {
    return sizeof(Arg) + get_args_sizes<CstringIdx>(c_string_sizes, args...);
  }
}

/**
 * Encode args to the buffer
 */
template <size_t CstringIdx>
constexpr std::byte* encode_args(size_t*, std::byte* out)
{
  return out;
}

template <size_t CstringIdx, typename Arg, typename... Args>
static inline constexpr std::byte* encode_args(size_t* c_string_sizes, std::byte* out, Arg&& arg, Args&&... args)
{
  out = detail::align_pointer<alignof(Arg), std::byte>(out);

  if constexpr (is_type_of_c_string<Arg>())
  {
    memcpy(out, arg, c_string_sizes[CstringIdx]);
    return encode_args<CstringIdx + 1>(c_string_sizes, out + c_string_sizes[CstringIdx],
                                       std::forward<Args>(args)...);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    size_t len = arg.size();
    memcpy(out, arg.data(), len);
    out[len] = static_cast<std::byte>(0);
    return encode_args<CstringIdx>(c_string_sizes, out + len + 1, std::forward<Args>(args)...);
  }
  else
  {
    // use memcpy when possible
    if constexpr (std::is_trivially_copyable_v<fmt::remove_cvref_t<Arg>>)
    {
      std::memcpy(out, &arg, sizeof(Arg));
    }
    else
    {
      new (out) fmt::remove_cvref_t<Arg>(std::forward<Arg>(arg));
    }

    return encode_args<CstringIdx>(c_string_sizes, out + sizeof(Arg), std::forward<Args>(args)...);
  }
}
} // namespace detail

/**
 * Stores the source metadata and additionally a span of TypeDescriptors
 */
struct Metadata
{
  /**
   * Creates and/or returns a pointer to Metadata with static lifetine
   * @tparam LogMacroMetadataFun
   * @tparam Args
   * @return
   */
  template <typename LogMacroMetadataFun, typename... Args>
  [[nodiscard]] static Metadata const* get() noexcept
  {
    static constexpr Metadata metadata{LogMacroMetadataFun{}(), detail::format_to<Args...>};
    return std::addressof(metadata);
  }

  constexpr Metadata(LogMacroMetadata macro_metadata, detail::FormatToFn format_to)
    : macro_metadata(macro_metadata), format_to_fn(format_to)
  {
  }

  LogMacroMetadata macro_metadata;
  detail::FormatToFn format_to_fn;
};

/**
 * A variable template that will call Metadata::get() during the program initialisation time
 */
template <typename LogMacroMetadataFun, typename... Args>
Metadata const* get_metadata_ptr{Metadata::get<LogMacroMetadataFun, Args...>()};
}; // namespace quill