# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(tasarchTests LANGUAGES CXX)

add_executable(tasarch_test source/tasarch_test.cpp)
target_compile_features(tasarch_test PRIVATE cxx_std_17)

add_test(NAME tasarch_test COMMAND tasarch_test)

add_folders(Test)
