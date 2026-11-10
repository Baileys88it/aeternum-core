# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Optional modules: every folder here with a CMakeLists.txt is added. A module decides itself whether it is built
# (its own option) and registers with
#   trinity_add_module(<library> <script loader function> [MANIFEST <module.manifest.json>] [CONFIGS <conf.dist files>])
# The loader function registers the module's scripts (WorldScript, PlayerScript, ...) like an AddSC_ function.
# worldserver links the `modules` library: its AddModuleScripts() calls every registered loader and is empty when
# no module is built. CONFIGS are copied to worldserver.conf.d/.
# MANIFEST (Aeternum M1-Blocco2, D9) is copied to <config dir>/modules/<id>/module.manifest.json and validated at boot.

function(trinity_add_module library loader)
  cmake_parse_arguments(MODULE "" "MANIFEST" "CONFIGS" ${ARGN})
  set_property(GLOBAL APPEND PROPERTY TRINITY_MODULE_LIBRARIES ${library})
  set_property(GLOBAL APPEND PROPERTY TRINITY_MODULE_LOADERS ${loader})
  set_property(GLOBAL APPEND PROPERTY TRINITY_MODULE_CONFIGS ${MODULE_CONFIGS})
  if(MODULE_MANIFEST)
    set_property(GLOBAL APPEND PROPERTY TRINITY_MODULE_MANIFESTS ${MODULE_MANIFEST})
  endif()
endfunction()

# SDK host must be configured before consumers (e.g. admin_monitoring).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/CMakeLists.txt)
  add_subdirectory(sdk)
endif()

file(GLOB module_dirs LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(module_dir ${module_dirs})
  get_filename_component(_mod_name ${module_dir} NAME)
  if(IS_DIRECTORY ${module_dir} AND EXISTS ${module_dir}/CMakeLists.txt AND NOT _mod_name STREQUAL "sdk")
    add_subdirectory(${module_dir})
  endif()
endforeach()

get_property(module_libraries GLOBAL PROPERTY TRINITY_MODULE_LIBRARIES)
get_property(module_loaders GLOBAL PROPERTY TRINITY_MODULE_LOADERS)

set(TRINITY_MODULE_LOADER_DECLARATIONS "")
set(TRINITY_MODULE_LOADER_CALLS "")
foreach(loader ${module_loaders})
  string(APPEND TRINITY_MODULE_LOADER_DECLARATIONS "void ${loader}();\n")
  string(APPEND TRINITY_MODULE_LOADER_CALLS "    ${loader}();\n")
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ModuleLoader.cpp.in.cmake ${CMAKE_CURRENT_BINARY_DIR}/ModuleLoader.cpp)

add_library(modules STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/ModuleLoader.cpp)

target_link_libraries(modules
  PRIVATE
    trinity-core-interface
  PUBLIC
    ${module_libraries})

set_target_properties(modules
  PROPERTIES
    FOLDER "server")

if(module_loaders)
  message(STATUS "Modules: ${module_loaders}")
endif()

# M1-Blocco5: fail the build if ABI interface headers pull in STL.
set(_aeternum_abi_stl_lint "${CMAKE_SOURCE_DIR}/contrib/aeternum/ci/check_abi_no_stl.ps1")
if(EXISTS "${_aeternum_abi_stl_lint}")
  add_custom_target(aeternum-abi-stl-lint ALL
    COMMAND PowerShell -NoProfile -ExecutionPolicy Bypass -File "${_aeternum_abi_stl_lint}" -RepoRoot "${CMAKE_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Aeternum: ABI no-STL lint (include/aeternum/abi + src/modules/**/abi)"
    VERBATIM)
  if(TARGET aeternum-sdk)
    add_dependencies(aeternum-sdk aeternum-abi-stl-lint)
  endif()
  if(TARGET modules)
    add_dependencies(modules aeternum-abi-stl-lint)
  endif()
endif()

