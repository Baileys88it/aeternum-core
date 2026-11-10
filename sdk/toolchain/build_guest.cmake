# Isolated driver: probe LLVM/Clang wasm32-unknown-unknown (no dummy guest payloads).
if(NOT EXISTS "${AETERNUM_WASM_CLANG}")
  message(FATAL_ERROR "AETERNUM_WASM_CLANG missing: '${AETERNUM_WASM_CLANG}'")
endif()
if(NOT IS_DIRECTORY "${AETERNUM_GUEST_SRC}")
  message(FATAL_ERROR "AETERNUM_GUEST_SRC missing")
endif()
file(MAKE_DIRECTORY "${AETERNUM_GUEST_BIN}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${AETERNUM_GUEST_SRC}" -B "${AETERNUM_GUEST_BIN}"
    "-DAETERNUM_WASM_CLANG=${AETERNUM_WASM_CLANG}"
  RESULT_VARIABLE _cfg)
if(NOT _cfg EQUAL 0)
  message(FATAL_ERROR "toolchain cmake configure failed")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${AETERNUM_GUEST_BIN}" --target aeternum_wasm_toolchain
  RESULT_VARIABLE _bld)
if(NOT _bld EQUAL 0)
  message(FATAL_ERROR "toolchain wasm32 probe failed")
endif()
