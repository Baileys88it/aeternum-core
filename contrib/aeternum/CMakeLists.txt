# Aeternum Engine - Windows runtime helper scripts
# configure_file generates bats/README into the build tree; worldserver/bnetserver
# POST_BUILD (see AttachRuntimeScripts.cmake) copies them into the per-config runtime dir.
# Also copies OpenSSL legacy.dll next to the executables (OpenSSLCrypto search path).

if(NOT WIN32)
  return()
endif()

set(AETERNUM_SCRIPTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(AETERNUM_SQL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sql")
set(AETERNUM_RUNTIME_GEN_DIR "${CMAKE_BINARY_DIR}/aeternum_runtime_gen")
file(MAKE_DIRECTORY "${AETERNUM_RUNTIME_GEN_DIR}")

set(AETERNUM_RUNTIME_GEN_DIR "${AETERNUM_RUNTIME_GEN_DIR}" CACHE INTERNAL "Generated Aeternum runtime helpers")

set(AETERNUM_CMAKE_VERSION "3.24+ (this tree configured with CMake 4.4.x)")
set(AETERNUM_VS_VERSION "Visual Studio 2022 / MSVC toolset v143 (14.44.x)")
set(AETERNUM_MYSQL_VERSION "MySQL Community Server 8.0.46 (8.0.x series), TCP port 3308")
set(AETERNUM_OPENSSL_VERSION "OpenSSL 3.5.9 (3.x series)")
set(AETERNUM_BOOST_VERSION "Boost 1.84")
set(AETERNUM_PYTHON_VERSION "Python 3.13.x (optional for contrib tools)")

# OpenSSL 3.x legacy provider DLL ? searched next to the exe by OpenSSLCrypto::threadsSetup
# (boost::dll::program_location().remove_filename()), same layout as Trinity Forever.
set(_aeternum_legacy_candidates "")
if(OPENSSL_ROOT_DIR)
  list(APPEND _aeternum_legacy_candidates
    "${OPENSSL_ROOT_DIR}/bin/legacy.dll"
    "${OPENSSL_ROOT_DIR}/bin/ossl-modules/legacy.dll"
    "${OPENSSL_ROOT_DIR}/lib/ossl-modules/legacy.dll")
endif()
if(OPENSSL_INCLUDE_DIR)
  get_filename_component(_aeternum_openssl_root "${OPENSSL_INCLUDE_DIR}" DIRECTORY)
  list(APPEND _aeternum_legacy_candidates
    "${_aeternum_openssl_root}/bin/legacy.dll"
    "${_aeternum_openssl_root}/bin/ossl-modules/legacy.dll"
    "${_aeternum_openssl_root}/lib/ossl-modules/legacy.dll")
endif()
list(APPEND _aeternum_legacy_candidates
  "C:/Program Files/OpenSSL-Win64/bin/legacy.dll")

set(AETERNUM_OPENSSL_LEGACY_DLL "")
foreach(_cand IN LISTS _aeternum_legacy_candidates)
  if(EXISTS "${_cand}")
    set(AETERNUM_OPENSSL_LEGACY_DLL "${_cand}")
    break()
  endif()
endforeach()

if(AETERNUM_OPENSSL_LEGACY_DLL)
  message(STATUS "Aeternum: OpenSSL legacy.dll -> ${AETERNUM_OPENSSL_LEGACY_DLL}")
  set(AETERNUM_OPENSSL_LEGACY_DLL "${AETERNUM_OPENSSL_LEGACY_DLL}" CACHE INTERNAL "OpenSSL 3.x legacy provider DLL")
else()
  message(WARNING "Aeternum: OpenSSL legacy.dll not found; runtime will warn 'OpenSSL failed to load legacy.dll'")
  set(AETERNUM_OPENSSL_LEGACY_DLL "" CACHE INTERNAL "OpenSSL 3.x legacy provider DLL")
endif()

foreach(_name IN ITEMS START_SERVER.bat STOP_SERVER.bat STATUS.bat CHECK_CONFIG.bat)
  configure_file(
    "${AETERNUM_SCRIPTS_SOURCE_DIR}/${_name}.in"
    "${AETERNUM_RUNTIME_GEN_DIR}/${_name}"
    @ONLY
    NEWLINE_STYLE WIN32
  )
endforeach()


# Instance-scoped STOP for isolated client QA (_testinst_iem1). Not copied next to
# worldserver by AttachRuntimeScripts; generated into aeternum_runtime_gen and the
# ops tree copies/syncs it to <ENGINE_ROOT>\_testinst_iem1\STOP_TESTINST.bat.
# Timeouts MUST stay 120s graceful / 180s hard (match STOP_SERVER.bat.in).
if(NOT AETERNUM_TOOLS_DIR)
  set(AETERNUM_TOOLS_DIR "${CMAKE_BINARY_DIR}/_tools" CACHE PATH "Path-scoped Aeternum helper scripts (Stop/Test-AlreadyRunning)")
endif()
if(NOT AETERNUM_TESTINST_BIN_DIR)
  set(AETERNUM_TESTINST_BIN_DIR "${CMAKE_BINARY_DIR}/_testinst_iem1/bin" CACHE PATH "Isolated testinst runtime bin dir")
endif()
configure_file(
  "${AETERNUM_SCRIPTS_SOURCE_DIR}/STOP_TESTINST.bat.in"
  "${AETERNUM_RUNTIME_GEN_DIR}/STOP_TESTINST.bat"
  @ONLY
  NEWLINE_STYLE WIN32
)

# Instance-scoped START for isolated client QA (_testinst_iem1). Echo ports MUST stay
# 3119/28083/28085 (isolata), never public 2119/18083/18085.
if(NOT AETERNUM_TESTINST_ROOT_DIR)
  set(AETERNUM_TESTINST_ROOT_DIR "${CMAKE_BINARY_DIR}/_testinst_iem1" CACHE PATH "Isolated testinst runtime root")
endif()
configure_file(
  "${AETERNUM_SCRIPTS_SOURCE_DIR}/START_TESTINST.bat.in"
  "${AETERNUM_RUNTIME_GEN_DIR}/START_TESTINST.bat"
  @ONLY
  NEWLINE_STYLE WIN32
)
# Path-scoped START/STOP helpers (shipped next to the bats; bats call %~dp0*.ps1)
foreach(_name IN ITEMS Test-AeternumAlreadyRunning.ps1 Stop-AeternumPathScoped.ps1 Manage-AeternumAdminToken.ps1 Deploy-AeternumModuleManifests.ps1)
  configure_file(
    "${AETERNUM_SCRIPTS_SOURCE_DIR}/${_name}"
    "${AETERNUM_RUNTIME_GEN_DIR}/${_name}"
    COPYONLY
  )
endforeach()


configure_file(
  "${AETERNUM_SCRIPTS_SOURCE_DIR}/README.txt.in"
  "${AETERNUM_RUNTIME_GEN_DIR}/README.txt"
  @ONLY
  NEWLINE_STYLE WIN32
)

configure_file(
  "${AETERNUM_SQL_SOURCE_DIR}/create_aeternum_dbs.sql"
  "${AETERNUM_RUNTIME_GEN_DIR}/create_aeternum_dbs.sql"
  COPYONLY
)

configure_file(
  "${AETERNUM_SQL_SOURCE_DIR}/set_realm_name_aeternum.sql"
  "${AETERNUM_RUNTIME_GEN_DIR}/set_realm_name_aeternum.sql"
  COPYONLY
)
