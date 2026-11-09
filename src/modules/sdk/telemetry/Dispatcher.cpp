#include "aeternum/sdk/internal/telemetry_http.hpp"
#include "aeternum/sdk/internal/profiler.hpp"
#include "aeternum/sdk/internal/wasm_abort.hpp"
#include "aeternum/sdk/internal/wasm_pipeline.hpp"
#include "AdminAuth.h"
#include "Errors.h" // same RapidJSON layout/assertion contract as Trinity consumers
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <charconv>
#include <cstdint>
#include <cstring>
namespace aeternum::sdk::internal::telemetry {
namespace {
Response Json(http::status code,std::string body) {
 Response out{code,11};out.set(http::field::content_type,"application/json");
 out.set(http::field::cache_control,"no-store");out.set("X-Content-Type-Options","nosniff");
 out.keep_alive(false);out.body()=std::move(body);out.prepare_payload();return out;
}
Response Error(http::status code,char const* message) {
 return Json(code,std::string("{\"error\":\"")+message+"\"}");
}
std::string Serialize(rapidjson::Document const& d) {
 rapidjson::StringBuffer buffer;rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
 d.Accept(writer);return {buffer.GetString(),buffer.GetSize()};
}
bool ExtractWasm(Request const& req,std::string& out) {
 auto type=std::string(req[http::field::content_type]);
 if(type=="application/wasm" || type=="application/octet-stream"){out=req.body();return !out.empty();}
 auto const prefix="multipart/form-data; boundary=";
 if(type.rfind(prefix,0)!=0)return false;
 auto boundary="--"+type.substr(std::char_traits<char>::length(prefix));
 auto const& body=req.body();auto pos=body.find(boundary);if(pos==std::string::npos)return false;
 auto headers=body.find("\r\n\r\n",pos);if(headers==std::string::npos)return false;
 auto start=headers+4;auto stop=body.find("\r\n"+boundary,start);if(stop==std::string::npos)return false;
 out.assign(body.data()+start,stop-start);return !out.empty();
}
bool WasmMagic(std::string const& bytes) {
 return bytes.size()>=8 && std::memcmp(bytes.data(),"\0asm",4)==0;
}
AdminMonitoring::Role Role(Request const& req,Config const& cfg) {
 if(req.count(http::field::authorization)!=1)return AdminMonitoring::Role::None;
 auto value=req[http::field::authorization];std::string provided;
 if(!AdminMonitoring::ExtractBearerToken(std::string_view(value.data(),value.size()),provided) || !ValidToken(provided))return AdminMonitoring::Role::None;
 auto equal=[&](std::string const& expected){return ValidToken(expected) && provided.size()==expected.size() && AdminMonitoring::ConstantTimeEqual(provided,expected);};
 if(equal(cfg.operatorToken))return AdminMonitoring::Role::Operator;
 if(equal(cfg.readToken))return AdminMonitoring::Role::ReadOnly;
 return AdminMonitoring::Role::None;
}
}
bool ValidToken(std::string const& value) {
 if(value.size()<32 || value.size()>256)return false;
 for(unsigned char c:value)if(c<=32 || c>=127)return false;
 return true;
}
Response Dispatch(Request const& req,Config const& cfg) {
 try {
 std::size_t headers=req.target().size()+16;
 for(auto const& field:req)headers+=field.name_string().size()+field.value().size()+4;
 if(headers>HeaderLimit)return Error(http::status::request_header_fields_too_large,"headers_too_large");
 auto const deploy=req.target()=="/api/wasm/deploy";
 if(req.body().size()>(deploy?WasmBodyLimit:BodyLimit))return Error(http::status::payload_too_large,"body_too_large");
 auto role=Role(req,cfg);
 if(role==AdminMonitoring::Role::None) {
  auto out=Error(http::status::unauthorized,"unauthorized");out.set(http::field::www_authenticate,"Bearer");return out;
 }
 // Native admin API: no browser origins, cookies, CORS, upgrades or chunking.
 if(req.count(http::field::origin) || req.count(http::field::cookie))return Error(http::status::forbidden,"browser_request_denied");
 if(req.version()!=11 || req.count(http::field::host)!=1 || req.count(http::field::transfer_encoding) ||
    req.count(http::field::upgrade) || req.count(http::field::expect) || req.count(http::field::content_length)>1)
  return Error(http::status::bad_request,"unsupported_framing");
 if(req.count(http::field::content_length)) {
  auto v=req[http::field::content_length];std::size_t size=0;
  auto parsed=std::from_chars(v.data(),v.data()+v.size(),size);
  if(parsed.ec!=std::errc() || parsed.ptr!=v.data()+v.size() || size!=req.body().size())return Error(http::status::bad_request,"invalid_length");
 } else if(!req.body().empty())return Error(http::status::bad_request,"length_required");
 if(req.target()=="/api/telemetry") {
  if(req.method()!=http::verb::get)return Error(http::status::method_not_allowed,"get_required");
  if(!req.body().empty())return Error(http::status::bad_request,"get_body_forbidden");
  // Independent core-owned C++ copy: no module HostAlloc context spoofing, no
  // C-ABI lifetime change. Never hold profiler/registry locks during socket I/O.
  auto json=AeternumProfiler::Instance().Dump();rapidjson::Document d;d.Parse(json.c_str());
  if(d.HasParseError() || !d.IsObject())return Error(http::status::internal_server_error,"telemetry_unavailable");
  auto& a=d.GetAllocator();rapidjson::Value modules(rapidjson::kArrayType);
  for(auto const& state:wasm::SnapshotModules()) {
   rapidjson::Value row(rapidjson::kObjectType);
   row.AddMember("module",rapidjson::Value(state.module.c_str(),static_cast<rapidjson::SizeType>(state.module.size()),a),a);
   row.AddMember("generation",state.generation,a);
   row.AddMember("state",rapidjson::Value(state.running?"running":"idle",a),a);
   row.AddMember("abortRequested",state.abortRequested,a);modules.PushBack(row,a);
  }
  d.AddMember("WasmModules",modules,a);return Json(http::status::ok,Serialize(d));
 }
 if(req.target()=="/api/wasm/deploy") {
  if(req.method()!=http::verb::post)return Error(http::status::method_not_allowed,"post_required");
  if(role!=AdminMonitoring::Role::Operator)return Error(http::status::forbidden,"operator_required");
  std::string image;if(!ExtractWasm(req,image) || !WasmMagic(image))return Error(http::status::unsupported_media_type,"wasm_required");
  if(!wasm::Pipeline::Instance().Deploy(reinterpret_cast<std::uint8_t const*>(image.data()),image.size()))
   return Error(http::status::unprocessable_entity,"wasm_load_failed");
  rapidjson::Document ok;ok.SetObject();auto& a=ok.GetAllocator();
  ok.AddMember("status","deployed",a);
  ok.AddMember("module",rapidjson::Value(wasm::HotIdentity,a),a);
  ok.AddMember("export",rapidjson::Value(wasm::HotExport,a),a);
  ok.AddMember("bytes",static_cast<std::uint64_t>(image.size()),a);
  return Json(http::status::ok,Serialize(ok));
 }
 if(req.target()!="/api/wasm/kill")return Error(http::status::not_found,"route_not_found");
 if(req.method()!=http::verb::post)return Error(http::status::method_not_allowed,"post_required");
 if(role!=AdminMonitoring::Role::Operator)return Error(http::status::forbidden,"operator_required");
 if(req.count(http::field::content_type)!=1 || req[http::field::content_type]!="application/json")return Error(http::status::unsupported_media_type,"json_required");
 rapidjson::Document d;d.Parse<rapidjson::kParseValidateEncodingFlag>(req.body().data(),req.body().size());
 if(d.HasParseError() || !d.IsObject() || d.MemberCount()!=2 || !d.HasMember("module") || !d["module"].IsString() ||
    !d.HasMember("generation") || !d["generation"].IsUint64() || !d["generation"].GetUint64())return Error(http::status::bad_request,"invalid_target");
 std::string module(d["module"].GetString(),d["module"].GetStringLength());auto generation=d["generation"].GetUint64();
 if(module.empty() || module.size()>1024 || module.find('\0')!=std::string::npos)return Error(http::status::bad_request,"invalid_module");
 auto result=wasm::RequestAbort(module,generation);
 switch(result) {
  case wasm::AbortResult::Missing:return Error(http::status::not_found,"module_not_found");
  case wasm::AbortResult::StaleGeneration:return Error(http::status::conflict,"stale_generation");
  case wasm::AbortResult::Idle:return Error(http::status::conflict,"module_idle");
  case wasm::AbortResult::AlreadyRequested:return Error(http::status::conflict,"abort_already_requested");
  case wasm::AbortResult::Accepted:break;
 }
 // 200 acknowledges the flag for THIS active generation, not VM destruction or
 // synchronous completion. A host callback cannot be preempted; completion may race.
 d.AddMember("status","abort_requested",d.GetAllocator());d.AddMember("cooperative",true,d.GetAllocator());
 d.AddMember("completed",false,d.GetAllocator());return Json(http::status::ok,Serialize(d));
 }catch(...){return Error(http::status::service_unavailable,"request_unavailable");}
}
}
