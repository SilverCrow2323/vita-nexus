cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain")
  else()
    message(FATAL_ERROR "Set VITASDK or pass CMAKE_TOOLCHAIN_FILE.")
  endif()
endif()

project(vita_nexus_native C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "VITA NEXUS")
set(VITA_TITLEID "VNXS00001")
set(VITA_VERSION "00.01")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME}
  vita2d
  SceCtrl_stub
  SceDisplay_stub
  SceGxm_stub
  SceLibKernel_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
