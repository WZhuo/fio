#pragma once

#include <expected>
#include <format>
#include <string>

namespace fio {

/// \brief Error types for iceberg.
enum class ErrorKind {
  kTimedOut,
  kNotSupported,
  kInvalidArgument,
};

/// \brief Error with a kind and a message.
struct [[nodiscard]] Error {
  ErrorKind kind;
  std::string message;
};

/// /brief Default error trait
template <typename T>
struct DefaultError {
  using type = Error;
};

/// \brief Result alias
template <typename T, typename E = typename DefaultError<T>::type>
using Result = std::expected<T, E>;

using Status = Result<void>;

/// \brief Macro to define error creation functions
#define DEFINE_ERROR_FUNCTION(name)                                           \
  template <typename... Args>                                                 \
  inline auto name(const std::format_string<Args...> fmt,                     \
                   Args&&... args) -> std::unexpected<Error> {                \
    return std::unexpected<Error>(                                            \
        {ErrorKind::k##name, std::format(fmt, std::forward<Args>(args)...)}); \
  }                                                                           \
  inline auto name(const std::string& message) -> std::unexpected<Error> {    \
    return std::unexpected<Error>({ErrorKind::k##name, message});             \
  }

DEFINE_ERROR_FUNCTION(TimedOut)
DEFINE_ERROR_FUNCTION(NotSupported)
DEFINE_ERROR_FUNCTION(InvalidArgument)

#undef DEFINE_ERROR_FUNCTION

}  // namespace fio
