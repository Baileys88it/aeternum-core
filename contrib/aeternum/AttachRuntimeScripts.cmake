# Included from worldserver / bnetserver CMakeLists (same directory scope as the target).
# Expects: AETERNUM_RUNTIME_TARGET, AETERNUM_RUNTIME_GEN_DIR (cache).
# Optionally copies AETERNUM_OPENSSL_LEGACY_DLL next to the exe (OpenSSLCrypto modules path).

if(NOT WIN32)
  return()
endif()

if(NOT AETERNUM_RUNTIME_GEN_DIR)
  message(STATUS "Aeternum: runtime gen dir not set yet; helper scripts will appear after reconfigure + rebuild")
  return()
endif()

if(NOT AETERNUM_RUNTIME_TARGET)
  message(FATAL_ERROR "AETERNUM_RUNTIME_TARGET must be set before including AttachRuntimeScripts.cmake")
endif()

if(NOT TARGET ${AETERNUM_RUNTIME_TARGET})
  return()
endif()

set(_aeternum_post_cmds
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/logs"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/START_SERVER.bat"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/START_SERVER.bat"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/STOP_SERVER.bat"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/STOP_SERVER.bat"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/STATUS.bat"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/STATUS.bat"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/CHECK_CONFIG.bat"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/CHECK_CONFIG.bat"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/Test-AeternumAlreadyRunning.ps1"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/Test-AeternumAlreadyRunning.ps1"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/Stop-AeternumPathScoped.ps1"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/Stop-AeternumPathScoped.ps1"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/Manage-AeternumAdminToken.ps1"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/Manage-AeternumAdminToken.ps1"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/Deploy-AeternumModuleManifests.ps1"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/Deploy-AeternumModuleManifests.ps1"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/README.txt"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/README.txt"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/create_aeternum_dbs.sql"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/create_aeternum_dbs.sql"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${AETERNUM_RUNTIME_GEN_DIR}/set_realm_name_aeternum.sql"
          "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/set_realm_name_aeternum.sql"
)

if(AETERNUM_OPENSSL_LEGACY_DLL)
  list(APPEND _aeternum_post_cmds
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AETERNUM_OPENSSL_LEGACY_DLL}"
            "$<TARGET_FILE_DIR:${AETERNUM_RUNTIME_TARGET}>/legacy.dll"
  )
endif()

add_custom_command(TARGET ${AETERNUM_RUNTIME_TARGET} POST_BUILD
  ${_aeternum_post_cmds}
  COMMENT "Aeternum: refresh runtime helper scripts beside ${AETERNUM_RUNTIME_TARGET}"
  VERBATIM
)
