cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(cd-rec-status VERSION 1.0.0)

include(compilerconfig)
include(defaults)
include(helpers)

set(SOURCES cd-rec-status.cpp)
set(CMAKE_PROJECT_NAME cd-rec-status)

add_library(${CMAKE_PROJECT_NAME} MODULE ${SOURCES})

find_package(Qt6 COMPONENTS Widgets Core WebSockets)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  OBS::libobs
  OBS::frontend-api
  Qt6::Core
  Qt6::Widgets
  Qt6::WebSockets
)

set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  "${CMAKE_SOURCE_DIR}/UI/obs-frontend-api")
set_target_properties_obs(${CMAKE_PROJECT_NAME} PROPERTIES FOLDER "plugins/noahvogt" PREFIX "")
