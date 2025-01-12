cmake_minimum_required(VERSION 3.29)
project(ZSTD)

set(CMAKE_CXX_STANDARD 23)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/ZSTD)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/ZSTD)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/ZSTD)

add_definitions(-DUNICODE -D_UNICODE)

# Create the shell extension DLL
add_library(ZSTDShellExt SHARED
    ShellExt.cpp
    ShellExt.h
    dllmain.cpp
    ShellExt.def
)

# Create registration utility
add_executable(RegisterExtension RegisterExtension.cpp)

# Add manifest to registration utility
if (MSVC)
    # Configure the manifest through a resource file
    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/RegisterExtension.manifest
            ${CMAKE_CURRENT_BINARY_DIR}/RegisterExtension.manifest
            COPYONLY
    )

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/manifest.rc
            "#define RT_MANIFEST 24\n"
            "#define APP_MANIFEST 1\n"
            "APP_MANIFEST RT_MANIFEST \"RegisterExtension.manifest\""
    )

    target_sources(RegisterExtension PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/manifest.rc
    )

    # Disable automatic manifest generation to avoid conflicts
    target_link_options(RegisterExtension PRIVATE "/MANIFEST:NO")
endif()

# Find ZSTD package
find_package(zstd CONFIG REQUIRED)

# Link against required libraries
target_link_libraries(ZSTDShellExt PRIVATE
    zstd::libzstd
    shlwapi.lib
    shell32.lib
    ole32.lib
    comctl32.lib
    uuid.lib
)

# Set DLL properties
set_target_properties(ZSTDShellExt PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    WINDOWS_EXPORT_ALL_SYMBOLS OFF  # Turn off automatic exports
)
