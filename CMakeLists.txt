cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable modern CMake policies
cmake_policy(VERSION 3.20)

# Set project name and version
project(MacType VERSION 2.0.0
        DESCRIPTION "Better font rendering for Windows"
        LANGUAGES CXX C)

# Windows only project
if(NOT WIN32)
    message(FATAL_ERROR "MacType only supports Windows platform")
endif()

# Include CMake built-in modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add cmake modules path for custom finders
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enable modern MSVC features
if(MSVC)
    # Use modern MSVC features
    add_compile_options(/Zc:__cplusplus)
    add_compile_options(/permissive-)
    add_compile_options(/Zc:throwingNew)
    add_compile_options(/Zc:referenceBinding)

    # Enable additional warnings
    add_compile_options(/W4)
    add_compile_options(/w14242 /w14254 /w14263 /w14265)
    add_compile_options(/w14287 /we4289 /w14296 /w14311)
    add_compile_options(/w14545 /w14546 /w14547 /w14549)
    add_compile_options(/w14555 /w14640 /w14826 /w14905)
    add_compile_options(/w14906 /w14928)
endif()

# Enable sanitizers if requested
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_TESTS "Enable building tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

if(ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()

if(ENABLE_UBSAN)
    if(MSVC)
        add_compile_options(/fsanitize=undefined)
        add_link_options(/fsanitize=undefined)
    else()
        add_compile_options(-fsanitize=undefined)
        add_link_options(-fsanitize=undefined)
    endif()
endif()

if(ENABLE_COVERAGE)
    if(MSVC)
        add_compile_options(/PROFILE)
    else()
        add_compile_options(-fprofile-arcs -ftest-coverage)
        add_link_options(-fprofile-arcs -ftest-coverage)
    endif()
endif()

# Find required packages (Windows-specific)
find_package(FreeType REQUIRED)
if(NOT FreeType_FOUND)
    message(FATAL_ERROR "FreeType not found. Please check vcpkg installation.")
endif()

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(BZip2 REQUIRED)
# Find Detours (Windows API hooking library)
# Detours doesn't provide standard CMake config files, find manually
find_path(DETOURS_INCLUDE_DIR detours/detours.h
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES include
    NO_DEFAULT_PATH
)

find_library(DETOURS_LIBRARY
    NAMES detours.lib detours
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH
)

if(NOT DETOURS_INCLUDE_DIR OR NOT DETOURS_LIBRARY)
    # Fallback: look in vcpkg installed directory (common paths)
    set(VCPKG_ROOT "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed")
    find_path(DETOURS_INCLUDE_DIR detours/detours.h
        PATHS
            "${VCPKG_ROOT}/x64-windows/include"
            "${VCPKG_ROOT}/x86-windows/include"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
            "$ENV{VCPKG_ROOT}/installed/x86-windows/include"
        NO_DEFAULT_PATH
    )
    find_library(DETOURS_LIBRARY
        NAMES detours.lib detours
        PATHS
            "${VCPKG_ROOT}/x64-windows/lib"
            "${VCPKG_ROOT}/x86-windows/lib"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
            "$ENV{VCPKG_ROOT}/installed/x86-windows/lib"
        NO_DEFAULT_PATH
    )
endif()

if(NOT DETOURS_INCLUDE_DIR OR NOT DETOURS_LIBRARY)
    message(FATAL_ERROR "Detours library not found. Please install detours via vcpkg.")
endif()

# Create interface library for detours
add_library(detours INTERFACE)
target_include_directories(detours INTERFACE ${DETOURS_INCLUDE_DIR})
target_link_libraries(detours INTERFACE ${DETOURS_LIBRARY})

# Note: Brotli is included statically in FreeType when brotli feature is enabled
# No separate brotli linking needed

# Find nlohmann_json for modern JSON handling
find_package(nlohmann_json REQUIRED)

# Define source files with new modular structure
set(MAC_TYPE_SOURCES
    # Core rendering engine
    src/core/ft.cpp
    src/core/fteng.cpp
    src/core/ft2vert.c
    src/core/ft_compat_ex.c
    src/core/ftref.c
    "src/core/ft - non-ref.cpp"
    src/core/override.cpp

    # Settings and configuration
    src/settings/settings.cpp

    # Font caching
    src/cache/cache.cpp
    src/cache/ownedcs.cpp

    # DLL loader and hooking
    src/platform/dll.cpp
    src/hooking/hook.cpp
    src/hooking/hookCounter.cpp

    # DirectWrite support
    src/platform/directwrite.cpp

    # Utility functions
    src/utils/common.cpp
    src/utils/misc.cpp
    src/utils/dynCodeHelper.cpp
    src/utils/EventLogging.cpp
    src/utils/expfunc.cpp
    src/utils/VersionHelper.cpp

    # Modern C++ utilities
    include/modern_cpp.cpp
)

# Test source files
set(MAC_TYPE_TEST_SOURCES
    tests/test_main.cpp
    tests/test_string_utils.cpp
    tests/test_settings.cpp
    tests/test_cache.cpp
    tests/test_performance.cpp
)

set(MAC_TYPE_HEADERS
    # Core headers
    src/core/ft.h
    src/core/fteng.h
    src/core/ft2vert.h
    src/core/ft2build.h
    src/core/ftref.h
    src/core/ivs_otft.h
    src/core/override.h

    # Settings and configuration
    src/settings/settings.h
    src/settings/strtoken.h

    # Font caching
    src/cache/cache.h
    src/cache/ownedcs.h

    # DLL loader and hooking
    src/platform/dll.h
    src/hooking/hookCounter.h
    src/hooking/hooklist.h

    # DirectWrite support
    src/platform/directwrite.h

    # Utility headers
    src/utils/common.h
    src/utils/dynCodeHelper.h
    include/modern_cpp.h

    # Event logging
    src/utils/EventLogging.h

    # Color management
    src/utils/colorinvert.h

    # Compatibility and system headers
    src/utils/sdk_compat.h
    src/utils/undocAPI.h
    src/utils/supinfo.h
    src/utils/array.h
    src/utils/hash_list.h
    src/utils/tlsdata.h
    src/utils/crc32.h
    src/utils/stdint.h
    src/utils/VersionHelper.h

    # Platform headers
    src/platform/gdiPlusFlat2.h
    src/platform/GdiPlusTypes2.h
    src/platform/resource.h
    src/platform/wow64ext.h
    src/platform/wow64layer.h

    # Hooking libraries
    src/hooking/detours.h
    src/hooking/easyhook.h
)

# Create main library
add_library(mactype ${MAC_TYPE_SOURCES} ${MAC_TYPE_HEADERS})

# Set target properties
set_target_properties(mactype PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(mactype PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(mactype PRIVATE
    ${FREETYPE_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${BROTLI_INCLUDE_DIRS}
    ${BZIP2_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(mactype PUBLIC
    FreeType::FreeType
    ZLIB::ZLIB
    PNG::PNG
    BZip2::BZip2
    nlohmann_json::nlohmann_json
    detours
    # Windows system libraries (required for Windows API hooking)
    user32
    gdi32
    dwrite
    uxtheme
    windowscodecs
    shlwapi
)

# Precompiled headers (optional, for faster compilation)
option(ENABLE_PCH "Enable precompiled headers" ON)
if(ENABLE_PCH)
    target_precompile_headers(mactype PRIVATE
        <windows.h>
        <string>
        <vector>
        <memory>
        <algorithm>
        <ft2build.h>
        <freetype/freetype.h>
        <freetype/ftcache.h>
    )
endif()

# Create executable targets
if(WIN32)
    # Main DLL
    add_executable(MacType.Core WIN32 src/platform/dll.cpp src/platform/resource)
    target_link_libraries(MacType.Core PRIVATE mactype)

    # GUI applications
    add_executable(MacTuner WIN32 src/platform/macloader.rc)
    target_link_libraries(MacTuner PRIVATE mactype)

    add_executable(MacTray WIN32 src/platform/macloader.rc)
    target_link_libraries(MacTray PRIVATE mactype)
endif()

# Install targets (exclude detours from export as it's an interface library)
install(TARGETS mactype
    EXPORT MacTypeTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Don't export detours as it's a local interface library

# Install headers
install(FILES ${MAC_TYPE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mactype
)

# Install executables
if(WIN32)
    install(TARGETS MacType.Core MacTuner MacTray
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Generate and install CMake config files
set(MAC_TYPE_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/MacType)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/MacTypeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MacTypeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/MacTypeConfig.cmake
    INSTALL_DESTINATION ${MAC_TYPE_CMAKE_DIR}
    PATH_VARS
        CMAKE_INSTALL_INCLUDEDIR
        CMAKE_INSTALL_LIBDIR
)

install(EXPORT MacTypeTargets
    FILE MacTypeTargets.cmake
    NAMESPACE MacType::
    DESTINATION ${MAC_TYPE_CMAKE_DIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/MacTypeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/MacTypeConfigVersion.cmake
    DESTINATION ${MAC_TYPE_CMAKE_DIR}
)

# Tests
if(BUILD_TESTING AND ENABLE_TESTS)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        message(WARNING "Google Test not found. Install with: vcpkg install gtest --feature=test")
        set(ENABLE_TESTS OFF CACHE BOOL "Enable tests" FORCE)
    endif()

    add_executable(mactype_tests ${MAC_TYPE_TEST_SOURCES})

    # Set test-specific properties
    set_target_properties(mactype_tests PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    # Test-specific include directories
    target_include_directories(mactype_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    # Test-specific compile definitions
    target_compile_definitions(mactype_tests PRIVATE
        TESTING_ENABLED
        _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
    )

    # Link libraries
    target_link_libraries(mactype_tests PRIVATE
        mactype
        GTest::gtest
        GTest::gtest_main
    )

    # Discover and configure tests
    include(GoogleTest)
    gtest_discover_tests(mactype_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES
            LABELS "unit"
            TIMEOUT 60
            ENVIRONMENT "GTEST_OUTPUT=xml:test_results.xml"
    )

    # Add custom test target
    add_custom_target(run_tests
        COMMAND mactype_tests --gtest_output=xml:test_results.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running MacType unit tests"
    )
endif()

# Benchmarks
if(ENABLE_TESTS)
    find_package(benchmark CONFIG QUIET)
    if(NOT benchmark_FOUND)
        message(WARNING "Google Benchmark not found. Install with: vcpkg install benchmark --feature=test")
        # 벤치마크 없이도 빌드 진행
    endif()

    if(benchmark_FOUND)
        set(BENCHMARK_SOURCES
            benchmarks/bench_font_rendering.cpp
            benchmarks/bench_cache.cpp
        )

        # 벤치마크 소스 파일이 존재하는지 확인
        set(EXISTING_BENCHMARK_SOURCES)
        foreach(src ${BENCHMARK_SOURCES})
            if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${src})
                list(APPEND EXISTING_BENCHMARK_SOURCES ${src})
            else()
                message(WARNING "Benchmark source file not found: ${src}")
            endif()
        endforeach()

        if(EXISTING_BENCHMARK_SOURCES)
            add_executable(mactype_benchmarks ${EXISTING_BENCHMARK_SOURCES})
            target_link_libraries(mactype_benchmarks PRIVATE
                mactype
                benchmark::benchmark
                benchmark::benchmark_main
            )
        else()
            message(WARNING "No benchmark source files found. Skipping benchmarks.")
        endif()
    endif()
endif()

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# Generate compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Print configuration summary
message(STATUS "=== MacType Configuration Summary ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Platform: Windows (Win32/x64)")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Enable ASan: ${ENABLE_ASAN}")
message(STATUS "Enable UBSan: ${ENABLE_UBSAN}")
message(STATUS "Enable coverage: ${ENABLE_COVERAGE}")
message(STATUS "Enable tests: ${ENABLE_TESTS}")
message(STATUS "Enable PCH: ${ENABLE_PCH}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  FreeType: ${FreeType_FOUND} (${FreeType_VERSION})")
message(STATUS "  ZLIB: ${ZLIB_FOUND}")
message(STATUS "  PNG: ${PNG_FOUND}")
message(STATUS "  BZip2: ${BZip2_FOUND}")
message(STATUS "  Detours: ${detours_FOUND}")
message(STATUS "  Note: Brotli is included statically in FreeType (brotli feature enabled)")
message(STATUS "  nlohmann_json: ${nlohmann_json_FOUND}")
message(STATUS "")
message(STATUS "Note: MacType is Windows-only software that hooks GDI/DirectWrite APIs")
message(STATUS "      to provide enhanced font rendering capabilities.")
message(STATUS "=====================================")
