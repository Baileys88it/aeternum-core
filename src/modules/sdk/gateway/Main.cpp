#include "aeternum/gateway/iocp_gateway.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int main(int argc,char** argv) {
 aeternum::gateway::Config cfg{};cfg.port=3725;cfg.workers=4;cfg.capacity=aeternum::gateway::kMaxCapacity;
 for(int i=1;i<argc;++i) {
  if(std::strcmp(argv[i],"--port")==0 && i+1<argc)cfg.port=static_cast<std::uint16_t>(std::atoi(argv[++i]));
  else if(std::strcmp(argv[i],"--bind")==0 && i+1<argc) {
   std::strncpy(cfg.bind,argv[++i],sizeof(cfg.bind)-1);cfg.bind[sizeof(cfg.bind)-1]=0;
  } else if(std::strcmp(argv[i],"--backend")==0 && i+1<argc && cfg.backendCount<aeternum::gateway::kMaxBackends) {
   auto* spec=argv[++i];auto* colon=std::strrchr(spec,':');if(!colon)return 2;
   auto& b=cfg.backends[cfg.backendCount++];
   auto n=static_cast<std::size_t>(colon-spec);if(n>=sizeof(b.address))return 2;
   std::memcpy(b.address,spec,n);b.address[n]=0;b.port=static_cast<std::uint16_t>(std::atoi(colon+1));
  }
 }
 if(!cfg.backendCount)return 2;
 aeternum::gateway::Gateway gw;if(!gw.Start(cfg))return 1;
 std::printf("aeternum_gateway listen=%u backends=%u capacity=%u\n",gw.BoundPort(),cfg.backendCount,cfg.capacity);
 std::fflush(stdout);
 for(;;) Sleep(1000);
}
