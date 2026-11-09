# Aeternum M7 Blocco1 - mod_combat_override: standalone native module sub-project (NOT part of the core build).
# Produces a pure mod_combat_override.dll / mod_combat_override.so that only includes the published C ABI header
# (include/aeternum/abi/AeternumABI.h); no link dependency on Trinity / Aeternum core.
#   cmake -S modules/native/mod_combat_override -B <build> && cmake --build <build> --config RelWithDebInfo
cmake_minimum_required(VERSION 3.18)
project(mod_combat_override LANGUAGES C)

get_filename_component(AETERNUM_ABI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../include" ABSOLUTE)
if(NOT EXISTS "${AETERNUM_ABI_INCLUDE_DIR}/aeternum/abi/AeternumABI.h")
  message(FATAL_ERROR "AeternumABI.h not found under ${AETERNUM_ABI_INCLUDE_DIR}")
endif()

add_library(mod_combat_override SHARED mod_combat_override.c)
target_include_directories(mod_combat_override PRIVATE "${AETERNUM_ABI_INCLUDE_DIR}")
target_compile_definitions(mod_combat_override PRIVATE AETERNUM_ABI_EXPORTS=1)
# Static CRT: the image depends on system DLLs only (loader uses LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | SYSTEM32).
set_target_properties(mod_combat_override
  PROPERTIES
    PREFIX ""
    C_STANDARD 11
    C_VISIBILITY_PRESET hidden
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(MSVC)
  target_compile_options(mod_combat_override PRIVATE /W4 /WX)
else()
  target_compile_options(mod_combat_override PRIVATE -Wall -Wextra -Werror)
endif()
