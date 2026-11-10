# Centered English welcome when a character enters the world.
option(BUILD_LOGIN_WELCOME "Build the centered login welcome module" 1)
if(NOT BUILD_LOGIN_WELCOME)
  return()
endif()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(login-welcome STATIC
  LoginWelcome.cpp)

target_link_libraries(login-welcome
  PRIVATE
    trinity-core-interface
  PUBLIC
    game-interface)

trinity_add_module(login-welcome AddSC_login_welcome
  CONFIGS
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/login_welcome.conf.dist)

message(STATUS "Module: Login welcome (centered raid-boss emote on character enter)")
