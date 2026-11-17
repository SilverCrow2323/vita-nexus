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
set(VITA_VERSION "00.07")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

set(NATIVE_CATALOG_INDEX "${CMAKE_CURRENT_SOURCE_DIR}/generated/catalog_index.c")
if(NOT EXISTS "${NATIVE_CATALOG_INDEX}")
  message(FATAL_ERROR "Native catalog index is missing. Run tools/catalog-builder/build_native_index.py first.")
endif()

add_executable(${PROJECT_NAME}
  src/main.c
  src/library_store.c
  src/profile_store.c
  ${NATIVE_CATALOG_INDEX}
)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME}
  vita2d
  SceCtrl_stub
  SceDisplay_stub
  SceGxm_stub
  SceSysmodule_stub
  ScePgf_stub
  ScePvf_stub
  SceCommonDialog_stub
  SceAppMgr_stub
  freetype
  png
  jpeg
  z
  m
  c
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/BarlowCondensed-Medium.ttf assets/fonts/BarlowCondensed-Medium.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/BarlowCondensed-Bold.ttf assets/fonts/BarlowCondensed-Bold.ttf
)
