# Aeternum Module SDK static host (M7) — no Trinity dynamic loading.
# Trinity coupling for ops facades lives ONLY under adapters/trinity (allowed seat).
option(BUILD_AETERNUM_MODULE_SDK "Build the Aeternum Module SDK static host" 1)
# IE-M3 Blocco3 (Wiring Finale): OS-level native module loader (.dll/.so via LoadLibraryExW/dlopen).
# Default ON (Blocco1/2 was OFF). Distinct from Trinity WITH_DYNAMIC_LINKING / ScriptReloadMgr (never used for SDK modules).
# OFF => ModuleHost never calls the OS loader (static IModule path unchanged, vanilla boot unchanged).
# ON  => compiled in, still disabled at runtime unless worldserver.conf Aeternum.Modules.NativeRuntime = 1.
# Empty runtime (ON + NativeRuntime=1 + zero native modules) = no-op fast path (hook count 0).
option(AETERNUM_NATIVE_MODULE_RUNTIME "IE-M3: OS-level native module loader (.dll/.so), default ON" ON)
if(NOT BUILD_AETERNUM_MODULE_SDK)
  return()
endif()

# Trinity-free native loader + C fault trampolines. Built when the runtime is ON
# (worldserver still requires Aeternum.Modules.NativeRuntime = 1 at boot).
add_library(aeternum-profiler STATIC profiler/AeternumProfiler.cpp)
target_include_directories(aeternum-profiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include)
set_target_properties(aeternum-profiler PROPERTIES FOLDER "server")

add_library(aeternum-ecs STATIC ecs/EcsKernel.cpp)
target_include_directories(aeternum-ecs PUBLIC ${CMAKE_SOURCE_DIR}/include)
set_target_properties(aeternum-ecs PROPERTIES FOLDER "server")

if(AETERNUM_NATIVE_MODULE_RUNTIME)
  if(WIN32)
    set(_aeternum_native_guard ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeGuardWin.c)
  else()
    set(_aeternum_native_guard ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeGuardPosix.c)
  endif()
  # IE-M3 Blocco2: NativeHost.cpp (module slots, HostAlloc cookies, tainted 60 s shutdown) +
  # NativeHooks.cpp (dense packet hook table staging / publish / guarded dispatch, hot path).
  add_library(aeternum-native-runtime STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeRuntime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeReload.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeHost.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeHooks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeSend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/NativeEcs.cpp
    ${_aeternum_native_guard})
  set_source_files_properties(${_aeternum_native_guard} PROPERTIES LANGUAGE C)
  target_include_directories(aeternum-native-runtime
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_SOURCE_DIR}/include
      ${CMAKE_SOURCE_DIR}/dep/rapidjson)
  target_link_libraries(aeternum-native-runtime
    PRIVATE
      trinity-core-interface
    PUBLIC
      aeternum-profiler
      aeternum-ecs
      openssl
      ${CMAKE_DL_LIBS})
  set_target_properties(aeternum-native-runtime PROPERTIES FOLDER "server")
endif()

add_subdirectory(wasm)

# M12 Phase2: Beast/Asio already supplied by Trinity; no fetched dependencies.
add_library(aeternum-telemetry-http STATIC telemetry/Dispatcher.cpp telemetry/AsyncServer.cpp)
target_include_directories(aeternum-telemetry-http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
 PRIVATE ${CMAKE_SOURCE_DIR}/src/modules/admin_monitoring ${CMAKE_SOURCE_DIR}/dep/rapidjson)
target_link_libraries(aeternum-telemetry-http PRIVATE trinity-core-interface common rapidjson PUBLIC boost aeternum-wasm aeternum-profiler)
set_target_properties(aeternum-telemetry-http PROPERTIES FOLDER "server")

# M13: standalone raw TCP relay, existing Boost only; no fetched dependencies.
add_library(aeternum-meshing STATIC meshing/AsyncEdge.cpp)
target_include_directories(aeternum-meshing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(aeternum-meshing PRIVATE trinity-core-interface PUBLIC boost)
set_target_properties(aeternum-meshing PROPERTIES FOLDER "server")

# M15: Trinity-free IOCP L7 gateway. Not linked into worldserver.
if(WIN32)
  add_library(aeternum-gateway STATIC gateway/IocpGateway.cpp)
  target_include_directories(aeternum-gateway PUBLIC ${CMAKE_SOURCE_DIR}/include)
  target_link_libraries(aeternum-gateway PRIVATE ws2_32 mswsock psapi)
  set_target_properties(aeternum-gateway PROPERTIES FOLDER "server")
  add_executable(aeternum_gateway gateway/Main.cpp)
  target_link_libraries(aeternum_gateway PRIVATE aeternum-gateway)
  set_target_properties(aeternum_gateway PROPERTIES OUTPUT_NAME aeternum_gateway FOLDER "server")
endif()

add_library(aeternum-sdk STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ModuleHost.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/adapters/trinity/TrinityOpsAdapter.cpp)

target_include_directories(aeternum-sdk
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/adapters/trinity
    ${CMAKE_SOURCE_DIR}/src/modules/admin_monitoring)

target_link_libraries(aeternum-sdk
  PRIVATE
    trinity-core-interface
  PUBLIC
    game-interface
    aeternum-wasm
    aeternum-telemetry-http
    aeternum-meshing)

if(AETERNUM_NATIVE_MODULE_RUNTIME)
  target_compile_definitions(aeternum-sdk PRIVATE AETERNUM_NATIVE_MODULE_RUNTIME=1)
  target_link_libraries(aeternum-sdk PRIVATE aeternum-native-runtime)
  message(STATUS "Module: Aeternum native module runtime (IE-M3) COMPILED IN (runtime switch Aeternum.Modules.NativeRuntime, default 0)")
else()
  message(STATUS "Module: Aeternum native module runtime (IE-M3) OFF (AETERNUM_NATIVE_MODULE_RUNTIME=OFF)")
endif()

# Operator overlay template (all flags off). Copied to worldserver.conf.d/ like admin_monitoring.conf.dist.
# *.conf.dist is not loaded; the operator copies it to aeternum_features.conf.
trinity_add_module(aeternum-sdk AddSC_aeternum_module_sdk
  CONFIGS
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/aeternum_features.conf.dist)

message(STATUS "Module: Aeternum Module SDK static host v0.2 (draft) + Trinity ops adapter")
