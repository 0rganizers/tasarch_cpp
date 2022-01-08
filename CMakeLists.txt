cmake_minimum_required(VERSION 3.18)

include(cmake/prelude.cmake)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")

project(
    tasarch
    VERSION 0.1.0
    DESCRIPTION "retroarch frontend for TASing"
    HOMEPAGE_URL "https://org.anize.rs"
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

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(cppcoro CONFIG REQUIRED)

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

# COMMON src and libs across all targets (testing, gui and "server")
set(COMMON_SRC_PATHS "${PROJECT_SOURCE_DIR}/src/log" "${PROJECT_SOURCE_DIR}/src/config" "${PROJECT_SOURCE_DIR}/src/gdb" "${PROJECT_SOURCE_DIR}/src/util")
set(COMMON_LIBS "fmt::fmt" "spdlog::spdlog" "toml11::toml11" "asio" "asio::asio" "cppcoro")

# src and libs for only gui
set(UI_SRC_PATHS "src" "src/gui" "src/test" ${COMMON_SRC_PATHS})
set(UI_LIBS "Qt6::Widgets" "Qt6::OpenGLWidgets" ${COMMON_LIBS})

# function for globbing the src files

function(glob_src_files src_paths out_cpp_files out_h_files)
  set(cpp_glob "")
  set(h_glob "")
  foreach(src_path IN LISTS src_paths)
    set(cpp_glob "${src_path}/*.cpp" ${cpp_glob})
    set(h_glob "${src_path}/*.h" "${src_path}/*.hpp" ${h_glob})
  endforeach()
  file(GLOB cpp_files ${cpp_glob})
  file(GLOB h_files ${h_glob})
  set(${out_cpp_files} ${cpp_files} PARENT_SCOPE)
  set(${out_h_files} ${h_files} PARENT_SCOPE)
endfunction()

glob_src_files("${UI_SRC_PATHS}" CPP_FILES H_FILES)
file(GLOB UI_FILES "src/gui/*.ui")

# ---- Declare executable ----

add_executable(tasarch_exe ${CPP_FILES} ${H_FILES} ${UI_FILES})
add_executable(tasarch::exe ALIAS tasarch_exe)

set_target_properties(
    tasarch_exe PROPERTIES
    OUTPUT_NAME tasarch
    EXPORT_NAME exe
)

target_compile_features(tasarch_exe PRIVATE cxx_std_20)
# set_property(TARGET tasarch_exe PROPERTY CXX_STANDARD 20)

source_group(TREE src PREFIX src FILES ${CPP_FILES} ${H_FILES} ${UI_FILES})

target_include_directories(tasarch_exe SYSTEM PUBLIC $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>)
target_include_directories(tasarch_exe PUBLIC $<BUILD_INTERFACE:${SRC_INCLUDE_DIR}>)

message(STATUS "Linking to libraries: ${UI_LIBS}")

target_link_libraries(tasarch_exe ${UI_LIBS})

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
