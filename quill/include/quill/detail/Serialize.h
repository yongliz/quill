/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "misc/Utilities.h"
#include "quill/LogLevel.h"
#include "quill/MacroMetadata.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Rdtsc.h"
#include "quill/detail/misc/TypeTraitsCopyable.h"
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace quill
{
namespace detail
{

/** Forward declaration **/
class LoggerDetails;

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_c_string()
{
  return fmt::detail::mapped_type_constant<Arg, fmt::format_context>::value == fmt::detail::type::cstring_type;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_string()
{
  return fmt::detail::mapped_type_constant<Arg, fmt::format_context>::value == fmt::detail::type::string_type;
}

template <typename Arg>
QUILL_NODISCARD inline constexpr bool need_call_dtor_for()
{
  using ArgType = detail::remove_cvref_t<Arg>;
  if constexpr (is_type_of_string<Arg>())
  {
    return false;
  }
  return !std::is_trivially_destructible<ArgType>::value;
}

template <bool ValueOnly, size_t Idx, size_t DestructIdx>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::byte* decode_args(std::byte* in,
                                                                  fmt::basic_format_arg<fmt::format_context>*,
                                                                  std::byte**)
{
  return in;
}

template <bool ValueOnly, size_t Idx, size_t DestructIdx, typename Arg, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::byte* decode_args(std::byte* in,
                                                                  fmt::basic_format_arg<fmt::format_context>* args,
                                                                  std::byte** destruct_args)
{
  using ArgType = detail::remove_cvref_t<Arg>;

  in = detail::align_pointer<alignof(Arg), std::byte>(in);

  if constexpr (is_type_of_c_string<Arg>() || is_type_of_string<Arg>())
  {
    std::string_view const v(reinterpret_cast<char const*>(in), strlen(reinterpret_cast<char const*>(in)));

    if constexpr (ValueOnly)
    {
      fmt::detail::value<fmt::format_context>& value_ =
        *reinterpret_cast<fmt::detail::value<fmt::format_context>*>(args + Idx);

      value_ = fmt::detail::arg_mapper<fmt::format_context>().map(v);
    }
    else
    {
      args[Idx] = fmt::detail::make_arg<fmt::format_context>(v);
    }
    return decode_args<ValueOnly, Idx + 1, DestructIdx, Args...>(in + v.length() + 1, args, destruct_args);
  }
  else
  {
    if constexpr (ValueOnly)
    {
      fmt::detail::value<fmt::format_context>& value_ =
        *(reinterpret_cast<fmt::detail::value<fmt::format_context>*>(args + Idx));

      value_ = fmt::detail::arg_mapper<fmt::format_context>().map(*reinterpret_cast<ArgType*>(in));
    }
    else
    {
      args[Idx] = fmt::detail::make_arg<fmt::format_context>(*reinterpret_cast<ArgType*>(in));
    }

    if constexpr (need_call_dtor_for<Arg>())
    {
      destruct_args[DestructIdx] = in;
      return decode_args<ValueOnly, Idx + 1, DestructIdx + 1, Args...>(in + sizeof(ArgType), args, destruct_args);
    }
    else
    {
      return decode_args<ValueOnly, Idx + 1, DestructIdx, Args...>(in + sizeof(ArgType), args, destruct_args);
    }
  }
}

template <size_t DestructIdx>
QUILL_ATTRIBUTE_HOT inline void destruct_args(std::byte**)
{
}

template <size_t DestructIdx, typename Arg, typename... Args>
QUILL_ATTRIBUTE_HOT inline void destruct_args(std::byte** args)
{
  using ArgType = detail::remove_cvref_t<Arg>;
  if constexpr (need_call_dtor_for<Arg>())
  {
    (reinterpret_cast<ArgType*>(args[DestructIdx]))->~ArgType();
    destruct_args<DestructIdx + 1, Args...>(args);
  }
  else
  {
    destruct_args<DestructIdx, Args...>(args);
  }
}

template <size_t CstringIdx>
QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr size_t get_args_sizes(size_t*)
{
  return 0;
}

/**
 * Get the size of all arguments
 */
template <size_t CstringIdx, typename Arg, typename... Args>
QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr size_t get_args_sizes(size_t* c_string_sizes,
                                                                  const Arg& arg, const Args&... args)
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
QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::byte* encode_args(size_t*, std::byte* out)
{
  return out;
}

template <size_t CstringIdx, typename Arg, typename... Args>
QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::byte* encode_args(size_t* c_string_sizes, std::byte* out,
                                                                   Arg&& arg, Args&&... args)
{
  out = detail::align_pointer<alignof(Arg), std::byte>(out);

  if constexpr (is_type_of_c_string<Arg>())
  {
    std::memcpy(out, arg, c_string_sizes[CstringIdx]);
    return encode_args<CstringIdx + 1>(c_string_sizes, out + c_string_sizes[CstringIdx],
                                       std::forward<Args>(args)...);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    std::memcpy(out, arg.data(), arg.length());
    out[arg.length()] = static_cast<std::byte>(0);
    return encode_args<CstringIdx>(c_string_sizes, out + arg.length() + 1, std::forward<Args>(args)...);
  }
  else
  {
    // use memcpy when possible
    if constexpr (std::is_trivially_copyable_v<detail::remove_cvref_t<Arg>>)
    {
      std::memcpy(out, &arg, sizeof(Arg));
    }
    else
    {
      new (out) detail::remove_cvref_t<Arg>(std::forward<Arg>(arg));
    }

    return encode_args<CstringIdx>(c_string_sizes, out + sizeof(Arg), std::forward<Args>(args)...);
  }
}

/**
 * Format function
 */
using FormatToFn = std::byte* (*)(fmt::string_view format, std::byte* data,
                                  quill::detail::FormatFnMemoryBuffer& out,
                                  std::vector<fmt::basic_format_arg<fmt::format_context>>& args);

template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* format_to(fmt::string_view format, std::byte* data,
                                                         quill::detail::FormatFnMemoryBuffer& out,
                                                         std::vector<fmt::basic_format_arg<fmt::format_context>>& args)
{
  constexpr size_t num_args = sizeof...(Args);
  constexpr size_t num_dtors = fmt::detail::count<need_call_dtor_for<Args>()...>();
  std::byte* dtor_args[std::max(num_dtors, (size_t)1)];
  std::byte* ret;

  auto argIdx = (int)args.size();
  args.resize(argIdx + num_args);
  ret = decode_args<false, 0, 0, Args...>(data, args.data() + argIdx, dtor_args);

  vformat_to(out, format, fmt::basic_format_args(args.data() + argIdx, num_args));
  destruct_args<0, Args...>(dtor_args);

  return ret;
}
} // namespace detail

/**
 * Stores the source metadata and additionally a span of TypeDescriptors
 */
struct Metadata
{
  /**
   * Creates and/or returns a pointer to Metadata with static lifetime
   * @tparam MacroMetadataFun
   * @tparam Args
   * @return
   */
  template <typename MacroMetadataFun, typename... Args>
  [[nodiscard]] static Metadata const* get() noexcept
  {
    static constexpr Metadata metadata{MacroMetadataFun{}(), detail::format_to<Args...>};
    return std::addressof(metadata);
  }

  constexpr Metadata(MacroMetadata macro_metadata, detail::FormatToFn format_to)
    : macro_metadata(macro_metadata), format_to_fn(format_to)
  {
  }

  MacroMetadata macro_metadata;
  detail::FormatToFn format_to_fn;
};

/**
 * A variable template that will call Metadata::get() during the program initialisation time
 */
template <typename MacroMetadataFun, typename... Args>
Metadata const* get_metadata_ptr{Metadata::get<MacroMetadataFun, Args...>()};

namespace detail
{
struct Header
{
public:
  Header() = default;
  Header(Metadata const* metadata, detail::LoggerDetails const* logger_details)
    : metadata(metadata), logger_details(logger_details){};

  Metadata const* metadata;
  detail::LoggerDetails const* logger_details;
#if !defined(QUILL_CHRONO_CLOCK)
  using using_rdtsc = std::true_type;
  uint64_t timestamp{detail::rdtsc()};
#else
  using using_rdtsc = std::false_type;
  uint64_t timestamp{static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count())};
#endif
};
} // namespace detail
} // namespace quill