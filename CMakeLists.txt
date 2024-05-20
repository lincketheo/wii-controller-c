# Pretty simple cmake here
cmake_minimum_required(VERSION 3.5)
project(wiis)


option(BUILD_WII_USE "Build wiiuse as well as wii-controller-c" OFF)
option(BUILD_EXE "Build an executable target" OFF)


if(${BUILD_WII_USE})
  # Wiiuse is a seperate third party application, sockets were getting a bit tricky and unreliable
  add_subdirectory(wiiuse)
  include_directories("${PROJECT_SOURCE_DIR}/wiiuse/src")
  set(WIIUSE wiiuse)
else()
  find_library(WIIUSE wiiuse)
endif()

find_library(PTHREAD pthread)
include_directories(include)

# Build wii-controller-library
file(GLOB INCLUDES "${CMAKE_PROJECT_DIR}/include/*.h")

set(LIB_SOURCES "${PROJECT_SOURCE_DIR}/src/robot_control.c" 
                "${PROJECT_SOURCE_DIR}/src/wii_controller.c" 
                "${PROJECT_SOURCE_DIR}/src/log.c" 
                "${PROJECT_SOURCE_DIR}/src/string_ops.c")


add_library(wii STATIC ${LIB_SOURCES})
target_link_libraries(wii ${WIIUSE})

if(${BUILD_EXE})
  add_executable(wii-controller-c "./src/main.c")
  target_link_libraries(wii-controller-c wii ${PTHREAD})
endif()

set_target_properties(wii PROPERTIES PUBLIC_HEADER "${INCLUDES}")
INSTALL(TARGETS wii
  LIBRARY DESTINATION "lib"
  PUBLIC_HEADER DESTINATION "include")

# For my vim plugin that reads compile comands, optional
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
