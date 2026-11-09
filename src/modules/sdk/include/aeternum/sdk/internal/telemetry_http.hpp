#pragma once
#include <boost/beast/http.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
namespace aeternum::sdk::internal::telemetry {
namespace http=boost::beast::http;
using Request=http::request<http::string_body>;
using Response=http::response<http::string_body>;
inline constexpr std::size_t HeaderLimit=8192, BodyLimit=2048, WasmBodyLimit=256*1024;
struct Config {
 bool enabled=false; std::string bind="127.0.0.1"; std::uint16_t port=0;
 std::string readToken,operatorToken; std::size_t maxConnections=16;
};
// Values are never logged. Tests use synthetic tokens, not account credentials.
bool ValidToken(std::string const& value);
Response Dispatch(Request const& request,Config const& config);
// Owner-thread Start/Stop/destruction. One I/O thread; all sockets/callbacks are
// drained before Stop returns. Start refuses non-loopback binds and missing auth.
class Server final {
public:
 Server();~Server();Server(Server const&)=delete;Server& operator=(Server const&)=delete;
 bool Start(Config config);void Stop();bool Running() const noexcept;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
