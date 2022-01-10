# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(tasarchTests LANGUAGES CXX)

set(TEST_SRC_PATHS "src" "src/config" "src/gdb" ${COMMON_SRC_PATHS})
set(TEST_LIBS ${COMMON_LIBS})

glob_src_files("${TEST_SRC_PATHS}" TEST_CPP_FILES TEST_H_FILES)

message(STATUS "test files: ${TEST_CPP_FILES}")

add_executable(tasarch_test ${TEST_CPP_FILES} ${TEST_H_FILES})
target_compile_features(tasarch_test PRIVATE cxx_std_20)

# set_target_properties(
#     tasarch_test PROPERTIES
#     CMAKE_AUTOMOC OFF
#     CMAKE_AUTORCC OFF
#     CMAKE_AUTOUIC OFF
# )

target_link_libraries(tasarch_test ${TEST_LIBS})
target_include_directories(tasarch_test SYSTEM PUBLIC $<BUILD_INTERFACE:${EXTERNAL_INCLUDE_DIR}>)
target_include_directories(tasarch_test PUBLIC $<BUILD_INTERFACE:${SRC_INCLUDE_DIR}>)

# always compile tests with coverage!
target_compile_options(tasarch_test PRIVATE "-Og" "-g" "--coverage")
target_link_options(tasarch_test PRIVATE "--coverage")

add_test(NAME tasarch_test COMMAND tasarch_test)

add_folders(Test)
