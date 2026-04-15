cmake_minimum_required(VERSION 3.18)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" TLCLIB_VERSION LIMIT_COUNT 1)

project(c_specx
    VERSION     "${TLCLIB_VERSION}"
    DESCRIPTION "Theo Lincke's standard c library and tools"
    LANGUAGES   C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(cmake/CompileConfig.cmake)
include(cmake/Options.cmake)
include(cmake/CompilerFlags.cmake)
include(cmake/Sanitizers.cmake)
add_subdirectory(src)

# Only build the test binary and package when c_specx is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(cmake/Packaging.cmake)

    if(NOT ENABLE_NTEST)
      add_executable(c_specx_test test.c)
      target_link_libraries(c_specx_test PRIVATE c_specx)
      target_include_directories(c_specx_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

      if(NOT WIN32)
          target_link_libraries(c_specx_test PRIVATE pthread)
      endif()

      install(TARGETS c_specx_test
          RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
          COMPONENT runtime
      )
    endif()
endif()
