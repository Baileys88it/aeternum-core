#include "aeternum/sdk/internal/wasm_pipeline.hpp"
namespace aeternum::sdk::internal::wasm {
Pipeline& Pipeline::Instance(){static Pipeline p;return p;}
bool Pipeline::Deploy(std::uint8_t const* data,std::size_t size){
 std::lock_guard<std::mutex> lock(mutex_);
 error_.clear();vm_.Reset();image_.clear();
 if(!data || size<8 || size>256*1024){error_="WASM image rejected";return false;}
 image_.assign(data,data+size);
 if(!vm_.Initialize({})){error_=vm_.Error();image_.clear();return false;}
 if(!vm_.LoadImage(image_.data(),image_.size(),HotIdentity)){
  error_=vm_.Error();vm_.Reset();image_.clear();return false;
 }
 if(!vm_.BindI32Export(HotExport)){error_=vm_.Error();vm_.Reset();image_.clear();return false;}
 return true;
}
bool Pipeline::Ready() const{std::lock_guard<std::mutex> lock(mutex_);return vm_.Ready() && !image_.empty();}
bool Pipeline::Tick(std::int32_t damage,std::int32_t& hp){
 std::lock_guard<std::mutex> lock(mutex_);
 if(!vm_.CallI32(HotExport,damage,hp)){error_=vm_.Error();return false;}
 return true;
}
void Pipeline::Reset() noexcept{
 std::lock_guard<std::mutex> lock(mutex_);
 vm_.Reset();std::vector<std::uint8_t>().swap(image_);error_.clear();
}
std::string const& Pipeline::Error() const{return error_;}
std::size_t Pipeline::ImageBytes() const{std::lock_guard<std::mutex> lock(mutex_);return image_.size();}
}
