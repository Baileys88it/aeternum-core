/*
 * Aeternum Engine public API — Status / Result (SDK 1.0 frozen surface).
 * Consumers: admin_monitoring, metric local sink, module host, protocol goldens.
 * FORBIDDEN in this tree: Trinity gameplay pointer types listed in docs/architecture/en/FORBIDDEN_INCLUDES.md.
 */
#ifndef AETERNUM_API_STATUS_HPP
#define AETERNUM_API_STATUS_HPP

namespace aeternum
{
enum class Status : int
{
    Ok = 0,
    InvalidArgument,
    NotFound,
    NotReady,
    Denied,
    Unsupported,
    Internal
};

template <class T>
struct Result
{
    Status status = Status::Internal;
    T value{};
    char const* message = nullptr;

    explicit operator bool() const { return status == Status::Ok; }
};
}

#endif
