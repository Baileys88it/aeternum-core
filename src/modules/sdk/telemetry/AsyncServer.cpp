#include "aeternum/sdk/internal/telemetry_http.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_set>
namespace aeternum::sdk::internal::telemetry {
namespace net=boost::asio;namespace beast=boost::beast;using tcp=net::ip::tcp;
struct Server::Impl {
 Config config;net::io_context io{1};tcp::acceptor acceptor{io};
 net::executor_work_guard<net::io_context::executor_type> work{io.get_executor()};
 std::thread worker;std::atomic<bool> running{false};bool stopping=false; // I/O thread only
 struct Session:std::enable_shared_from_this<Session> {
  Impl& owner;  beast::tcp_stream stream;beast::flat_buffer buffer{HeaderLimit+WasmBodyLimit+4096};
  http::request_parser<http::string_body> parser;Response response;bool closed=false;
  Session(Impl& o,tcp::socket socket):owner(o),stream(std::move(socket)) {
   parser.header_limit(static_cast<std::uint32_t>(HeaderLimit));parser.body_limit(WasmBodyLimit);
  }
  void Close() {
   if(closed)return;closed=true;boost::system::error_code ec;
   stream.socket().cancel(ec);stream.socket().shutdown(tcp::socket::shutdown_both,ec);stream.socket().close(ec);
   owner.sessions.erase(shared_from_this());
  }
  void Read() {
   stream.expires_after(std::chrono::seconds(5));auto self=shared_from_this();
   http::async_read(stream,buffer,parser,[self](boost::system::error_code ec,std::size_t) {
    if(ec || self->owner.stopping){self->Close();return;}
    try {
     self->response=Dispatch(self->parser.get(),self->owner.config);
     self->stream.expires_after(std::chrono::seconds(5));
     http::async_write(self->stream,self->response,[self](boost::system::error_code,std::size_t){self->Close();});
    }catch(...){self->Close();}
   });
  }
 };
 std::unordered_set<std::shared_ptr<Session>> sessions; // accessed on I/O thread only
 void Shutdown() {
  if(stopping)return;stopping=true;boost::system::error_code ec;acceptor.cancel(ec);acceptor.close(ec);
  while(!sessions.empty()){auto session=*sessions.begin();session->Close();}work.reset();
 }
 void Accept() {
  acceptor.async_accept([this](boost::system::error_code ec,tcp::socket socket) {
   if(stopping)return;
   try {
    if(!ec && sessions.size()<config.maxConnections) {
     auto session=std::make_shared<Session>(*this,std::move(socket));sessions.insert(session);session->Read();
    } // overflow socket RAII closes; at most maxConnections parsers/responses.
    if(!ec)Accept();else Shutdown();
   }catch(...){Shutdown();}
  });
 }
 void Run() noexcept {
  try {Accept();io.run();}
  catch(...) {Shutdown();try{io.run();}catch(...){}}
  running.store(false,std::memory_order_release);
 }
};
Server::Server()=default;Server::~Server(){Stop();}
bool Server::Start(Config config) {
 if(impl_ || !config.enabled || !config.port || !ValidToken(config.operatorToken) ||
    config.maxConnections<1 || config.maxConnections>64)return false;
 try {
  boost::system::error_code ec;auto address=net::ip::make_address(config.bind,ec);
  if(ec || !address.is_loopback())return false;
  auto p=std::make_unique<Impl>();p->config=std::move(config);
  tcp::endpoint endpoint(address,p->config.port);
  p->acceptor.open(endpoint.protocol(),ec);if(ec)return false;
  p->acceptor.bind(endpoint,ec);if(ec)return false; // no SO_REUSEADDR weakening on Windows
  p->acceptor.listen(static_cast<int>(p->config.maxConnections),ec);if(ec)return false;
  p->running.store(true,std::memory_order_release);
  p->worker=std::thread([state=p.get()]{state->Run();});impl_=std::move(p);return true;
 }catch(...){return false;}
}
void Server::Stop() {
 if(!impl_)return;
 // Only lifecycle owner calls Stop, never an I/O callback; no detached workers.
 try {net::post(impl_->io,[state=impl_.get()]{state->Shutdown();});}
 catch(...) {impl_->io.stop();} // allocation failure: quiesce first, then close on owner
 if(impl_->worker.joinable())impl_->worker.join();
 impl_->Shutdown();impl_->io.restart();
 try {impl_->io.run();}catch(...){impl_->io.stop();}
 impl_.reset();
}
bool Server::Running() const noexcept {return impl_ && impl_->running.load(std::memory_order_acquire);}
}
