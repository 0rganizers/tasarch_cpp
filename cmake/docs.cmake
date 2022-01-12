# ---- Redefine docs_early_return ----

# This function must be a macro, so the return() takes effect in the calling
# scope. This prevents other targets from being available and potentially
# requiring dependencies. This cuts down on the time it takes to generate
# documentation in CI.
macro(docs_early_return)
  return()
endmacro()

# ---- Dependencies ----

# include(FetchContent)
# FetchContent_Declare(
#     mcss URL
#     https://github.com/friendlyanon/m.css/releases/download/release-1/mcss.zip
#     URL_MD5 00cd2757ebafb9bcba7f5d399b3bec7f
#     SOURCE_DIR "${PROJECT_BINARY_DIR}/mcss"
#     UPDATE_DISCONNECTED YES
# )
# FetchContent_MakeAvailable(mcss)

find_package(Python3 3.6 REQUIRED)

# ---- Declare documentation target ----

set(
    DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs"
    CACHE PATH "Path for the generated Doxygen documentation"
)

set(working_dir "${PROJECT_BINARY_DIR}/docs")

foreach(file IN ITEMS Doxyfile conf.py)
  configure_file("docs/${file}.in" "${working_dir}/${file}" @ONLY)
endforeach()

set(mcss_SOURCE_DIR "${PROJECT_SOURCE_DIR}/docs/m.css")
set(mcss_script "${mcss_SOURCE_DIR}/documentation/doxygen.py")
set(config "${working_dir}/conf.py")

set(
    DOCS_COVERAGE_HTML_COMMAND
    genhtml --legend -f -q
    "${PROJECT_BINARY_DIR}/docs_coverage.info"
    -p "${PROJECT_SOURCE_DIR}"
    -o "${PROJECT_BINARY_DIR}/docs_coverage_html"
    CACHE STRING
    "; separated command to generate an HTML report for the 'coverage' target"
)

# TODO: with the bash below this is not portable, but who would want to compile the docs on windows anyways.
add_custom_target(
    docs
    COMMAND "${CMAKE_COMMAND}" -E remove_directory
    "${DOXYGEN_OUTPUT_DIRECTORY}/html"
    "${DOXYGEN_OUTPUT_DIRECTORY}/xml"
    COMMAND "bash" "-c" "cd ${mcss_SOURCE_DIR}/css && ./postprocess.sh"
    COMMAND "${Python3_EXECUTABLE}" "${mcss_script}" "${config}"
    COMMAND "${Python3_EXECUTABLE}" "-m" "coverxygen" "--xml-dir" "${DOXYGEN_OUTPUT_DIRECTORY}/xml" "--src-dir" "${PROJECT_SOURCE_DIR}" "--output" "${PROJECT_BINARY_DIR}/docs_coverage.info" "--prefix" "${PROJECT_SOURCE_DIR}/src" "--kind" "enum,enumvalue,friend,typedef,variable,function,signal,slot,class,struct,union,define,page"
    COMMAND ${DOCS_COVERAGE_HTML_COMMAND}
    COMMENT "Building documentation using Doxygen and m.css"
    WORKING_DIRECTORY "${working_dir}"
    VERBATIM
)
