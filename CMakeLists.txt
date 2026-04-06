cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(cd-rec-status VERSION 1.1.0)

include(compilerconfig)
include(defaults)
include(helpers)

set(SOURCES cd-rec-status.cpp)
set(CMAKE_PROJECT_NAME cd-rec-status)

add_library(${CMAKE_PROJECT_NAME} MODULE ${SOURCES})

find_package(obs-frontend-api REQUIRED)
find_package(Qt6 COMPONENTS Widgets Core)

include(FetchContent)

FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)

FetchContent_Declare(
  websocketpp
  GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
  GIT_TAG        0.8.2
)
FetchContent_MakeAvailable(websocketpp)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ASIO_STANDALONE)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Core
  Qt6::Widgets
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  "${CMAKE_SOURCE_DIR}/UI/obs-frontend-api"
  ${asio_SOURCE_DIR}/asio/include
  ${websocketpp_SOURCE_DIR}
)
set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES FOLDER "plugins/noahvogt" PREFIX "")
