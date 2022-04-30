#pragma once

#include "quill/LogMacroMetadata.h"
#include "quill/detail/LoggerDetails.h"

namespace quill
{
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