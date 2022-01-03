cmake_minimum_required(VERSION 3.18)

include(cmake/prelude.cmake)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")

project(
    tasarch
    VERSION 0.1.0
    DESCRIPTION "retroarch frontend for TASing"
    HOMEPAGE_URL "https://retroc.tf"
    LANGUAGES CXX
)

docs_early_return()

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ---- External Dependencies ----

add_subdirectory(external)
set(EXTERNAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(SRC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# for QT
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Core5Compat OpenGLWidgets REQUIRED)

# find_package(PkgConfig REQUIRED)

# pkg_check_modules(ELPP REQUIRED easyloggingpp)

# ---- Declare library ----

# add_library(
#     tasarch_lib OBJECT
#     source/lib.cpp
# )

# target_include_directories(
#     tasarch_lib ${warning_guard}
#     PUBLIC
#     "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
# )

# target_compile_features(tasarch_lib PUBLIC cxx_std_17)

# ---- Gather Files ----

file(GLOB CPP_FILES "src/*.cpp" "src/gui/*.cpp" "src/test/*.cpp")
file(GLOB H_FILES "src/*.h" "src/gui/*.h" "src/test/*.h")
file(GLOB UI_FILES "src/gui/*.ui")

# ---- Declare executable ----

add_executable(tasarch_exe MACOSX_BUNDLE src/main.cpp $<TARGET_OBJECTS:elpp> ${CPP_FILES} ${H_FILES} ${UI_FILES})
add_executable(tasarch::exe ALIAS tasarch_exe)

set_target_properties(
    tasarch_exe PROPERTIES
    OUTPUT_NAME tasarch
    EXPORT_NAME exe
)

target_compile_features(tasarch_exe PRIVATE cxx_std_17)

source_group(TREE src PREFIX src FILES ${CPP_FILES} ${H_FILES} ${UI_FILES})

target_include_directories(tasarch_exe SYSTEM PUBLIC $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>)
target_include_directories(tasarch_exe PUBLIC $<BUILD_INTERFACE:${SRC_INCLUDE_DIR}>)
target_link_libraries(tasarch_exe Qt6::Widgets Qt6::OpenGLWidgets)

message(STATUS "qt: ${Qt6_INCLUDE_DIRS}")

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT tasarch_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of tasarch"
  )
endif()

include(cmake/dev-mode.cmake)
