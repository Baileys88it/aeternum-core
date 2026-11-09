#include "aeternum/sdk/internal/native_reload.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
#if defined(_WIN32)
#include <windows.h>
#include <array>
#include <string_view>
#endif
namespace aeternum::sdk::internal::native {
namespace {
constexpr std::uint64_t Closed=std::uint64_t{1}<<63;
std::atomic<std::uint64_t> admission{0};std::atomic<bool> writer{false};
thread_local unsigned depth=0;thread_local bool writeOwner=false;
}
NativeReadLease::NativeReadLease() noexcept {
 if(writeOwner || depth){++depth;active_=true;return;}
 auto current=admission.load();
 while(!(current&Closed))if(admission.compare_exchange_weak(current,current+1)) {root_=active_=true;++depth;return;}
}
NativeReadLease::~NativeReadLease(){if(active_){--depth;if(root_)admission.fetch_sub(1);}}
bool TryNativeWrite() noexcept {
 if(depth || writeOwner)return false;
 admission.fetch_or(Closed);
 if(admission.load()!=Closed)return false;
 bool expected=false;if(!writer.compare_exchange_strong(expected,true))return false;
 writeOwner=true;return true;
}
void CancelNativePause() noexcept {if(!writer.load())admission.fetch_and(~Closed);}
void EndNativeWrite() noexcept {writeOwner=false;admission.store(0);writer.store(false);}
struct NativeFileWatcher::Impl {
 std::mutex mutex;std::condition_variable changed;bool dirty=false,stopped=true,armed=false;
 std::thread worker;
#if defined(_WIN32)
 HANDLE directory=INVALID_HANDLE_VALUE,stop=nullptr,ready=nullptr;
 void Run() noexcept {
  alignas(FILE_NOTIFY_INFORMATION) std::array<unsigned char,16384> bytes{};
  for(;;) {
   OVERLAPPED operation{};operation.hEvent=ready;ResetEvent(ready);
   if(!ReadDirectoryChangesW(directory,bytes.data(),DWORD(bytes.size()),TRUE,
       FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_SIZE,nullptr,&operation,nullptr))break;
   {std::lock_guard<std::mutex> lock(mutex);armed=true;changed.notify_all();}
   HANDLE events[]{stop,ready};auto outcome=WaitForMultipleObjects(2,events,FALSE,INFINITE);
   if(outcome!=WAIT_OBJECT_0+1) {
    CancelIoEx(directory,&operation);DWORD ignored=0;GetOverlappedResult(directory,&operation,&ignored,TRUE);break;
   }
   DWORD count=0;if(!GetOverlappedResult(directory,&operation,&count,FALSE))break;
   bool relevant=count==0; // overflow: conservatively rescan all existing reloadable modules
   for(DWORD offset=0;offset<count;) {
    auto const* event=reinterpret_cast<FILE_NOTIFY_INFORMATION const*>(bytes.data()+offset);
    if(offset+offsetof(FILE_NOTIFY_INFORMATION,FileName)+event->FileNameLength>count) {relevant=true;break;}
    std::wstring_view name(event->FileName,event->FileNameLength/sizeof(wchar_t));
    // Atomic rename (.next -> .dll) and trusted manifest replacement are both observed.
    auto ends=[&](std::wstring_view suffix){return name.size()>=suffix.size() && CompareStringOrdinal(name.data()+name.size()-suffix.size(),int(suffix.size()),suffix.data(),int(suffix.size()),TRUE)==CSTR_EQUAL;};
    if(ends(L".dll") || ends(L".so") || ends(L"module.manifest.json"))relevant=true;
    if(!event->NextEntryOffset)break;offset+=event->NextEntryOffset;
   }
   if(relevant){std::lock_guard<std::mutex> lock(mutex);dirty=true;changed.notify_all();}
  }
  {std::lock_guard<std::mutex> lock(mutex);stopped=true;changed.notify_all();}
 }
#endif
};
NativeFileWatcher::NativeFileWatcher():impl_(std::make_unique<Impl>()){}
NativeFileWatcher::~NativeFileWatcher(){Stop();}
bool NativeFileWatcher::Start(std::filesystem::path const& root) {
 Stop();auto& s=*impl_;
#if defined(_WIN32)
 s.directory=CreateFileW(root.c_str(),FILE_LIST_DIRECTORY,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
   nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
 if(s.directory==INVALID_HANDLE_VALUE)return false;
 s.stop=CreateEventW(nullptr,TRUE,FALSE,nullptr);s.ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 if(!s.stop || !s.ready){Stop();return false;}
 {std::lock_guard<std::mutex> lock(s.mutex);s.stopped=false;s.dirty=false;s.armed=false;}
 try{s.worker=std::thread([&s]{s.Run();});}catch(...){Stop();return false;}
 {std::unique_lock<std::mutex> lock(s.mutex);if(s.changed.wait_for(lock,std::chrono::seconds(3),[&s]{return s.armed||s.stopped;}) && s.armed)return true;}
 Stop();return false;
#else
 (void)root;return false; // no polling fallback disguised as native .so event support
#endif
}
void NativeFileWatcher::Stop() {
 auto& s=*impl_;
#if defined(_WIN32)
 if(s.stop)SetEvent(s.stop);
 if(s.worker.joinable())s.worker.join();
 if(s.directory!=INVALID_HANDLE_VALUE)CloseHandle(s.directory);s.directory=INVALID_HANDLE_VALUE;
 if(s.stop)CloseHandle(s.stop);s.stop=nullptr;if(s.ready)CloseHandle(s.ready);s.ready=nullptr;
#endif
 {std::lock_guard<std::mutex> lock(s.mutex);s.stopped=true;s.changed.notify_all();}
}
bool NativeFileWatcher::TakeDirty(){auto& s=*impl_;std::lock_guard<std::mutex> lock(s.mutex);bool value=s.dirty;s.dirty=false;return value;}
bool NativeFileWatcher::WaitDirty(std::chrono::milliseconds timeout){auto& s=*impl_;std::unique_lock<std::mutex> lock(s.mutex);return s.changed.wait_for(lock,timeout,[&s]{return s.dirty||s.stopped;}) && s.dirty;}
}
