# Gather base sources
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB_RECURSE POSIX_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/intf/os_posix/*.c")
file(GLOB_RECURSE WINDOWS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/intf/os_windows/*.c")

# Start with all sources, strip apps and both platform dirs
set(TLCLIB_SOURCES ${ALL_SOURCES})
list(REMOVE_ITEM TLCLIB_SOURCES ${POSIX_SOURCES})    # Don't include platform
list(REMOVE_ITEM TLCLIB_SOURCES ${WINDOWS_SOURCES})

# If tests are disabled (NTEST defined), remove test sources
if(ENABLE_NTEST)
    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c")
    if(TEST_SOURCES)
        list(REMOVE_ITEM TLCLIB_SOURCES ${TEST_SOURCES})
    endif()
endif()

# Add back the correct platform
if (WIN32)
    list(APPEND TLCLIB_SOURCES ${WINDOWS_SOURCES})
else ()
    list(APPEND TLCLIB_SOURCES ${POSIX_SOURCES})
endif ()

# The main library
add_library(c_specx OBJECT ${TLCLIB_SOURCES})
target_include_directories(c_specx
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
)
# Add c11 option
if (MSVC)
    target_compile_options(c_specx PRIVATE /std:c11)
else ()
    target_compile_features(c_specx PRIVATE c_std_11)
endif ()
if (WIN32)
    target_include_directories(c_specx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/intf/os_windows")
    target_link_libraries(c_specx PUBLIC ws2_32)
else ()
    target_include_directories(c_specx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/intf/os_posix")
    target_link_libraries(c_specx PUBLIC pthread)
endif ()
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT development
        FILES_MATCHING PATTERN "*.h"
)
install(
        FILES
        "${PROJECT_SOURCE_DIR}/README.md"
        "${PROJECT_SOURCE_DIR}/LICENSE.md"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
        COMPONENT runtime
        OPTIONAL
)
