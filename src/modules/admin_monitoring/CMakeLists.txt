# Aeternum Admin & Monitoring module â€” Boost.Beast, localhost-only (SDK consumer).
option(BUILD_ADMIN_MONITORING "Build the Admin & Monitoring module" 1)
if(NOT BUILD_ADMIN_MONITORING)
  return()
endif()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(admin-monitoring)

CollectAndAddSourceFiles(
  admin-monitoring
  ${CMAKE_CURRENT_SOURCE_DIR}
  EXCLUDE
  ${CMAKE_CURRENT_SOURCE_DIR}/conf)

target_include_directories(admin-monitoring
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/modules/sdk/include
    ${CMAKE_SOURCE_DIR}/include)

# Headers from sdk/; RegisterModule etc. resolved when worldserver links aeternum-sdk.
target_link_libraries(admin-monitoring
  PRIVATE
    trinity-core-interface
  PUBLIC
    game-interface)

trinity_add_module(admin-monitoring AddSC_admin_monitoring
  MANIFEST
    ${CMAKE_CURRENT_SOURCE_DIR}/module.manifest.json
  CONFIGS
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/admin_monitoring.conf.dist)

message(STATUS "Module: Admin & Monitoring (SDK static consumer, read-only v0)")
