if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_INSTALL_INCLUDEDIR include/tasarch CACHE PATH "")
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package tasarch)

install(
    TARGETS tasarch_exe
    EXPORT tasarchTargets
    BUNDLE	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    RUNTIME COMPONENT tasarch_Runtime
)

# for now???
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

# I have no idea what this does, maybe it works lmao?
set( APPS "\${CMAKE_INSTALL_PREFIX}/tasarch.app" )
set( DIRS "\${CMAKE_INSTALL_PREFIX}/lib" )
set(qtconf_dest_dir tasarch.app/Contents/Resources)
install(CODE "
file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt.conf\" \"\")
" COMPONENT Runtime)
install( CODE "
include(BundleUtilities)
fixup_bundle( \"${APPS}\" \"\" \"${DIRS}\" )
" Component Runtime )

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    tasarch_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(tasarch_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${tasarch_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT tasarch_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${tasarch_INSTALL_CMAKEDIR}"
    COMPONENT tasarch_Development
)

install(
    EXPORT tasarchTargets
    NAMESPACE tasarch::
    DESTINATION "${tasarch_INSTALL_CMAKEDIR}"
    COMPONENT tasarch_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
