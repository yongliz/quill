/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/TweakMe.h"
#include <sstream>
#include <string>

/**
 * Common type definitions etc
 */
#if !defined(QUILL_ACTIVE_LOG_LEVEL)
  #define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3
#endif

/**
 * Convert number to string
 */
#define QUILL_AS_STR(x) #x
#define QUILL_STRINGIFY(x) QUILL_AS_STR(x)

/**
 * Likely
 */
#if defined(__GNUC__)
  #define QUILL_LIKELY(x) (__builtin_expect((x), 1))
  #define QUILL_UNLIKELY(x) (__builtin_expect((x), 0))
#else
  #define QUILL_LIKELY(x) (x)
  #define QUILL_UNLIKELY(x) (x)
#endif

/**
 * Require check
 */
#define QUILL_REQUIRE(expression, error)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (QUILL_UNLIKELY(!(expression)))                                                             \
    {                                                                                              \
      printf("Quill fatal error: %s (%s:%d)\n", error, __FILE__, __LINE__);                        \
      std::abort();                                                                                \
    }                                                                                              \
  } while (0)

namespace quill
{
namespace detail
{
using FormatFnMemoryBuffer = fmt::basic_memory_buffer<char, 1024>;
} // namespace detail

/**
 * filename_t for windows/linux
 */
#if defined(_WIN32)
using filename_t = std::wstring;
using filename_ss_t = std::wstringstream;
  #define QUILL_FILENAME_STR(s) L##s
#else
using filename_t = std::string;
using filename_ss_t = std::stringstream;
  #define QUILL_FILENAME_STR(s) s
#endif

// Path delimiter
/**
 * Path delimiter windows/linux
 */
#if defined(_WIN32)
static constexpr char path_delimiter = '\\';
#else
static constexpr char path_delimiter = '/';
#endif

/**
 * Enum to select a timezone
 */
enum class Timezone : uint8_t
{
  LocalTime,
  GmtTime
};

} // namespace quill
