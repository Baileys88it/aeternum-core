# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(worldserver
  ${PRIVATE_SOURCES}
)

CollectAndAddSourceFiles(
  worldserver
  ${CMAKE_CURRENT_SOURCE_DIR}
  EXCLUDE
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if(WIN32)
  if(MSVC)
    target_sources(worldserver PRIVATE worldserver.rc)
  endif()
else()
  target_compile_definitions(worldserver PRIVATE
    _TRINITY_CORE_CONFIG="${CONF_DIR}/worldserver.conf"
    _TRINITY_CORE_CONFIG_DIR="${CONF_DIR}/worldserver.conf.d"
  )
endif()

target_link_libraries(worldserver
  PRIVATE
    trinity-core-interface
  PUBLIC
    scripts
    game
    readline
    gsoap
  PRIVATE
    modules)

target_include_directories(worldserver
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(worldserver
  PROPERTIES
    COMPILE_WARNING_AS_ERROR ${WITH_WARNINGS_AS_ERRORS}
    FOLDER "server")

# Add all dynamic projects as dependency to the worldserver
if(WORLDSERVER_DYNAMIC_SCRIPT_MODULES_DEPENDENCIES)
  add_dependencies(worldserver ${WORLDSERVER_DYNAMIC_SCRIPT_MODULES_DEPENDENCIES})
endif()

# Module configs (trinity_add_module CONFIGS) go to worldserver.conf.d/, which is loaded after worldserver.conf
get_property(module_configs GLOBAL PROPERTY TRINITY_MODULE_CONFIGS)

# Aeternum M1-Blocco2 (D9): module manifests go to <config dir>/modules/<id>/module.manifest.json (read once at boot).
get_property(module_manifests GLOBAL PROPERTY TRINITY_MODULE_MANIFESTS)
set(aeternum_manifest_ids "")
foreach(manifest ${module_manifests})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${manifest})
  file(READ ${manifest} _aeternum_manifest_json)
  string(JSON _aeternum_manifest_id ERROR_VARIABLE _aeternum_manifest_err GET "${_aeternum_manifest_json}" id)
  if(_aeternum_manifest_err OR NOT _aeternum_manifest_id MATCHES "^[a-z0-9_-]+(\\.[a-z0-9_-]+)*$")
    message(FATAL_ERROR "Invalid module manifest ${manifest}: id '${_aeternum_manifest_id}' ${_aeternum_manifest_err}")
  endif()
  list(APPEND aeternum_manifest_ids ${_aeternum_manifest_id})
endforeach()

if(UNIX)
  install(TARGETS worldserver DESTINATION bin)
  if(COPY_CONF)
    add_custom_command(TARGET worldserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/worldserver.conf.dist ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/worldserver.conf.dist
    )
    install(FILES worldserver.conf.dist DESTINATION ${CONF_DIR})
    if(module_configs)
      add_custom_command(TARGET worldserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/worldserver.conf.d
        COMMAND ${CMAKE_COMMAND} -E copy ${module_configs} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/worldserver.conf.d/
      )
      install(FILES ${module_configs} DESTINATION ${CONF_DIR}/worldserver.conf.d)
    endif()
    foreach(manifest _aeternum_id IN ZIP_LISTS module_manifests aeternum_manifest_ids)
      add_custom_command(TARGET worldserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/modules/${_aeternum_id}
        COMMAND ${CMAKE_COMMAND} -E copy ${manifest} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/modules/${_aeternum_id}/module.manifest.json
      )
      install(FILES ${manifest} DESTINATION ${CONF_DIR}/modules/${_aeternum_id})
    endforeach()
    # Operator modules/README.md (not src/modules/README.md). Same folder the loader reads.
    add_custom_command(TARGET worldserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/modules
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/contrib/aeternum/modules/README.md ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../etc/modules/README.md
    )
    install(FILES ${CMAKE_SOURCE_DIR}/contrib/aeternum/modules/README.md DESTINATION ${CONF_DIR}/modules)
  endif()
elseif(WIN32)
  install(TARGETS worldserver DESTINATION "${CMAKE_INSTALL_PREFIX}")
  if(COPY_CONF)
    add_custom_command(TARGET worldserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/worldserver.conf.dist ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
    )
    install(FILES worldserver.conf.dist DESTINATION "${CMAKE_INSTALL_PREFIX}")
    if(module_configs)
      add_custom_command(TARGET worldserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/worldserver.conf.d
        COMMAND ${CMAKE_COMMAND} -E copy ${module_configs} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/worldserver.conf.d/
      )
      install(FILES ${module_configs} DESTINATION "${CMAKE_INSTALL_PREFIX}/worldserver.conf.d")
    endif()
    foreach(manifest _aeternum_id IN ZIP_LISTS module_manifests aeternum_manifest_ids)
      add_custom_command(TARGET worldserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/modules/${_aeternum_id}
        COMMAND ${CMAKE_COMMAND} -E copy ${manifest} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/modules/${_aeternum_id}/module.manifest.json
      )
      install(FILES ${manifest} DESTINATION "${CMAKE_INSTALL_PREFIX}/modules/${_aeternum_id}")
    endforeach()
    # Operator modules/README.md (not src/modules/README.md). Same folder the loader reads.
    add_custom_command(TARGET worldserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/modules
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/contrib/aeternum/modules/README.md ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/modules/README.md
    )
    install(FILES ${CMAKE_SOURCE_DIR}/contrib/aeternum/modules/README.md DESTINATION "${CMAKE_INSTALL_PREFIX}/modules")
  endif()
endif()

# Generate precompiled header
if(USE_COREPCH)
  add_cxx_pch(worldserver PrecompiledHeaders/worldPCH.h)
endif()

# Aeternum: copy START/STOP/STATUS/CHECK_CONFIG + README into runtime output dir
if(WIN32)
  set(AETERNUM_RUNTIME_TARGET worldserver)
  include(${CMAKE_SOURCE_DIR}/contrib/aeternum/AttachRuntimeScripts.cmake)
endif()
