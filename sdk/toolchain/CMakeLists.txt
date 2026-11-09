# Isolated guest toolchain. Not add_subdirectory'd into the MSVC Trinity host tree.
cmake_minimum_required(VERSION 3.24)
project(aeternum_wasm_toolchain NONE)
if(NOT EXISTS "${AETERNUM_WASM_CLANG}")
  message(FATAL_ERROR "toolchain CMake requires AETERNUM_WASM_CLANG (LLVM/Clang)")
endif()
get_filename_component(_wasm_bin "${AETERNUM_WASM_CLANG}" DIRECTORY)
if(EXISTS "${_wasm_bin}/clang++.exe")
  set(_wasm_cxx "${_wasm_bin}/clang++.exe")
elseif(EXISTS "${_wasm_bin}/clang++")
  set(_wasm_cxx "${_wasm_bin}/clang++")
else()
  set(_wasm_cxx "${AETERNUM_WASM_CLANG}")
endif()
message(STATUS "Aeternum WASM toolchain: ${_wasm_cxx} --target=wasm32-unknown-unknown -ffreestanding -nostdlib")
add_custom_target(aeternum_wasm_toolchain ALL
  COMMAND "${_wasm_cxx}" --target=wasm32-unknown-unknown --version
  COMMENT "LLVM/Clang wasm32-unknown-unknown toolchain probe"
  VERBATIM)
