#pragma once

/**
 * C++ language standard detection
 */
#if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17 == 1) // fix for issue #464
  #define QUILL_HAS_CPP_17
  #define QUILL_HAS_CPP_14
#elif (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(_HAS_CXX14) && _HAS_CXX14 == 1)
  #define QUILL_HAS_CPP_14
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
 * Always Inline
 */
#if defined(__GNUC__)
  #define QUILL_ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
  #define QUILL_ALWAYS_INLINE __forceinline
#else
  #define QUILL_ALWAYS_INLINE inline
#endif

/**
* __has_attribute
*/
#ifdef __has_attribute
  #define QUILL_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
  #define QUILL_HAS_ATTRIBUTE(x) 0
#endif

/**
* __has_cpp_attribute
*/
#if defined(__cplusplus) && defined(__has_cpp_attribute)
  #define QUILL_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
  #define QUILL_HAS_CPP_ATTRIBUTE(x) 0
#endif

/**
 * Portable no discard warnings
 */
#if QUILL_HAS_CPP_ATTRIBUTE(nodiscard)
  #if defined(__clang__) && !defined(QUILL_HAS_CPP_17)
    #define QUILL_NODISCARD
  #else
    #define QUILL_NODISCARD [[nodiscard]]
  #endif
#elif QUILL_HAS_CPP_ATTRIBUTE(gnu::warn_unused_result)
  #define QUILL_NODISCARD [[gnu::warn_unused_result]]
#else
  #define QUILL_NODISCARD
#endif

/**
* Portable maybe_unsed
*/
#if QUILL_HAS_CPP_ATTRIBUTE(maybe_unused)
  #define QUILL_MAYBE_UNUSED [[maybe_unused]]
#elif QUILL_HAS_ATTRIBUTE(__unused__) || defined(__GNUC__)
  #define QUILL_MAYBE_UNUSED __attribute__((__unused__))
#else
  #define QUILL_MAYBE_UNUSED
#endif

/**
* Gcc hot/cold attributes
* Tells GCC that a function is hot or cold. GCC can use this information to
* improve static analysis, i.e. a conditional branch to a cold function
* is likely to be not-taken.
*/
#if QUILL_HAS_ATTRIBUTE(hot) || (defined(__GNUC__) && !defined(__clang__))
  #define QUILL_ATTRIBUTE_HOT __attribute__((hot))
#else
  #define QUILL_ATTRIBUTE_HOT
#endif

#if QUILL_HAS_ATTRIBUTE(cold) || (defined(__GNUC__) && !defined(__clang__))
  #define QUILL_ATTRIBUTE_COLD __attribute__((cold))
#else
  #define QUILL_ATTRIBUTE_COLD
#endif