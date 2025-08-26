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

# Find FreeType manually (vcpkg compatibility)
if(VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
    find_path(FREETYPE_INCLUDE_DIR ft2build.h
        PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/freetype2
              ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include
        NO_DEFAULT_PATH
    )

    find_library(FREETYPE_LIBRARY
        NAMES freetype.lib freetype
        PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
        NO_DEFAULT_PATH
    )
else()
    # Fallback: try to find in CMAKE_PREFIX_PATH or standard paths
    find_path(FREETYPE_INCLUDE_DIR ft2build.h
        PATHS ${CMAKE_PREFIX_PATH}/include/freetype2
              ${CMAKE_PREFIX_PATH}/include
              $ENV{VCPKG_ROOT}/installed/x64-windows/include/freetype2
              $ENV{VCPKG_ROOT}/installed/x64-windows/include
              $ENV{VCPKG_ROOT}/installed/x86-windows/include/freetype2
              $ENV{VCPKG_ROOT}/installed/x86-windows/include
              $ENV{VCPKG_ROOT}/installed/arm64-windows/include/freetype2
              $ENV{VCPKG_ROOT}/installed/arm64-windows/include
    )

    find_library(FREETYPE_LIBRARY
        NAMES freetype.lib freetype
        PATHS ${CMAKE_PREFIX_PATH}/lib
              $ENV{VCPKG_ROOT}/installed/x64-windows/lib
              $ENV{VCPKG_ROOT}/installed/x86-windows/lib
              $ENV{VCPKG_ROOT}/installed/arm64-windows/lib
    )
endif()

if(NOT FREETYPE_INCLUDE_DIR OR NOT FREETYPE_LIBRARY)
    message(FATAL_ERROR "FreeType not found. Please check vcpkg installation.")
endif()

# Create FreeType target
add_library(FreeType::FreeType UNKNOWN IMPORTED)
set_target_properties(FreeType::FreeType PROPERTIES
    IMPORTED_LOCATION "${FREETYPE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}"
)

message(STATUS "FreeType found: ${FREETYPE_LIBRARY}")
message(STATUS "FreeType include: ${FREETYPE_INCLUDE_DIR}")

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(BZip2 REQUIRED)

# Optional: Check for wow64ext (32/64-bit process communication)
# If not available, wow64ext functionality will be disabled
find_library(WOW64EXT_LIBRARY
    NAMES wow64ext.lib wow64ext
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH
)
if(WOW64EXT_LIBRARY)
    message(STATUS "Found wow64ext: ${WOW64EXT_LIBRARY}")
    add_compile_definitions(HAS_WOW64EXT)
else()
    message(WARNING "wow64ext not found. 32/64-bit process communication features will be disabled.")
endif()

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
            "${VCPKG_ROOT}/arm64-windows/include"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
            "$ENV{VCPKG_ROOT}/installed/x86-windows/include"
            "$ENV{VCPKG_ROOT}/installed/arm64-windows/include"
        NO_DEFAULT_PATH
    )
    find_library(DETOURS_LIBRARY
        NAMES detours.lib detours
        PATHS
            "${VCPKG_ROOT}/x64-windows/lib"
            "${VCPKG_ROOT}/x86-windows/lib"
            "${VCPKG_ROOT}/arm64-windows/lib"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
            "$ENV{VCPKG_ROOT}/installed/x86-windows/lib"
            "$ENV{VCPKG_ROOT}/installed/arm64-windows/lib"
        NO_DEFAULT_PATH
    )
endif()

if(NOT DETOURS_INCLUDE_DIR OR NOT DETOURS_LIBRARY)
    message(WARNING "Detours library not found. Windows API hooking features will be disabled.")
    message(WARNING "Install detours via: vcpkg install detours")
    set(HAS_DETOURS FALSE)
else()
    set(HAS_DETOURS TRUE)
    message(STATUS "Detours found: ${DETOURS_LIBRARY}")

    # Create imported library for detours
    add_library(detours STATIC IMPORTED)
    set_target_properties(detours PROPERTIES
        IMPORTED_LOCATION "${DETOURS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${DETOURS_INCLUDE_DIR}"
    )
endif()

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
    # Windows system libraries (required for Windows API hooking)
    user32
    gdi32
    dwrite
    uxtheme
)

# Conditionally link detours if available
if(HAS_DETOURS)
    target_link_libraries(mactype PUBLIC detours)
    target_compile_definitions(mactype PUBLIC HAS_DETOURS)
endif()

# Additional Windows libraries
target_link_libraries(mactype PUBLIC
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
    # Main DLL - MacType.Core as DLL (traditional architecture)
    add_library(MacType.Core SHARED src/platform/dll.cpp src/platform/resource)
    target_link_libraries(MacType.Core PRIVATE mactype)

    # 아키텍처별 DLL 이름 설정
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
        set_target_properties(MacType.Core PROPERTIES OUTPUT_NAME "MacType64")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
        set_target_properties(MacType.Core PROPERTIES OUTPUT_NAME "MacType_ARM64")
    else()
        set_target_properties(MacType.Core PROPERTIES OUTPUT_NAME "MacType")
    endif()

    # Legacy GUI applications - excluded from build
    # TODO: Replace with modern WinUI 3 applications (MacWiz and MacTuner)
    # add_executable(MacTuner WIN32 src/platform/macloader.rc)
    # add_executable(MacTray WIN32 src/platform/macloader.rc)
    # add_executable(MacType.Loader WIN32 src/platform/run.cpp)

    # Modern mt64agnt with WinUI 3
    option(BUILD_MODERN_AGENT "Build the modern mt64agnt with WinUI 3" ON)
    if(BUILD_MODERN_AGENT)
        # Find Windows App SDK (only on Windows)
        if(WIN32)
            # Try to find Windows App SDK in common installation paths
            set(WINDOWS_APP_SDK_PATHS
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.4_*"
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.3_*"
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.2_*"
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.1_*"
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.0_*"
            )

            find_path(MICROSOFT_WINDOWS_APP_SDK_INCLUDE_DIR
                NAMES Microsoft.UI.Xaml.Markup.h
                PATHS ${WINDOWS_APP_SDK_PATHS}
                PATH_SUFFIXES include
            )

            find_library(MICROSOFT_WINDOWS_APP_SDK_LIBRARY
                NAMES Microsoft.WindowsAppSDK.lib
                PATHS ${WINDOWS_APP_SDK_PATHS}
                PATH_SUFFIXES lib
            )

            if(MICROSOFT_WINDOWS_APP_SDK_INCLUDE_DIR AND MICROSOFT_WINDOWS_APP_SDK_LIBRARY)
                set(Microsoft_WindowsAppSDK_FOUND TRUE)
                add_library(Microsoft::WindowsAppSDK INTERFACE IMPORTED)
                set_target_properties(Microsoft::WindowsAppSDK PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${MICROSOFT_WINDOWS_APP_SDK_INCLUDE_DIR}"
                    INTERFACE_LINK_LIBRARIES "${MICROSOFT_WINDOWS_APP_SDK_LIBRARY}"
                )
                message(STATUS "Found Windows App SDK: ${MICROSOFT_WINDOWS_APP_SDK_INCLUDE_DIR}")
            else()
                message(WARNING "Windows App SDK not found. Modern agent will not be built.")
                set(BUILD_MODERN_AGENT OFF)
            endif()
        endif()

        # Collect agent source files
        set(AGENT_SOURCES
            src/platform/agent/mt64agnt.cpp
            src/platform/agent/core/agent_controller.cpp
            src/platform/agent/core/process_bridge.cpp
            src/platform/agent/ipc/secure_channel.cpp
            src/platform/agent/ipc/named_pipe_server.cpp
            src/platform/agent/injection/modern_injector.cpp
            src/platform/agent/utils/logger.cpp
            src/platform/agent/utils/winrt_helper.cpp
        )

        # 플랫폼별로 다른 타겟 이름 설정
        if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
            set(AGENT_TARGET_NAME "mt64agnt-arm64")
            set(WINAPP_PLATFORM "ARM64")
        elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
            set(AGENT_TARGET_NAME "mt64agnt-x64")
            set(WINAPP_PLATFORM "x64")
        else()
            set(AGENT_TARGET_NAME "mt64agnt-x64")
            set(WINAPP_PLATFORM "x64")
        endif()

        # Add WinUI 3 executable with platform-specific name
        add_executable(${AGENT_TARGET_NAME} WIN32 ${AGENT_SOURCES})

        # Set WinUI 3 properties
        set_target_properties(${AGENT_TARGET_NAME} PROPERTIES
            VS_GLOBAL_WindowsAppSDKBootstrapAutoInitializeOptions_Platform ${WINAPP_PLATFORM}
            VS_GLOBAL_WindowsAppSDKBootstrapAutoInitializeOptions_Version 1.4
            VS_PACKAGE_REFERENCES "Microsoft.WindowsAppSDK_1.4"
        )

        # Link dependencies
        target_link_libraries(${AGENT_TARGET_NAME}
            PRIVATE
            mactype
            Microsoft::WindowsAppSDK
        )

        # Include directories
        target_include_directories(${AGENT_TARGET_NAME} PRIVATE
            src/platform/agent
            src
        )

        # Precompiled headers for better build performance
        if(ENABLE_PCH)
            target_precompile_headers(${AGENT_TARGET_NAME} PRIVATE
                <winrt/Windows.Foundation.h>
                <winrt/Windows.Security.Cryptography.h>
                <memory>
                <string>
                <vector>
            )
        endif()

        # Set C++/WinRT properties
        set_target_properties(${AGENT_TARGET_NAME} PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
        )
    endif()
endif()

# Install targets - do not export mactype to avoid detours dependency issues
install(TARGETS mactype
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(FILES ${MAC_TYPE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mactype
)

# Install executables
if(WIN32)
    install(TARGETS MacType.Core  # Legacy GUI apps excluded - modern WinUI 3 apps will replace them
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install modern agent if built
    if(BUILD_MODERN_AGENT)
        if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
            install(TARGETS mt64agnt-arm64
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        else()
            install(TARGETS mt64agnt-x64
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
    endif()
endif()

# Modern MacType Manager - Universal WinUI 3 App
option(BUILD_MODERN_MANAGER "Build the modern MacType Manager with WinUI 3 (Universal App)" ON)
if(BUILD_MODERN_MANAGER)
    # Find Windows App SDK (only on Windows)
    if(WIN32)
        # Windows App SDK is already found above for mt64agnt
        if(NOT Microsoft_WindowsAppSDK_FOUND)
            # Try to find Windows App SDK again for manager
            set(WINDOWS_APP_SDK_PATHS
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.4_*"
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.3_*"
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.2_*"
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.1_*"
                "$ENV{ProgramFiles}/WindowsApps/Microsoft.WindowsAppSDK.1.0_*"
            )

            find_path(MICROSOFT_WINDOWS_APP_SDK_INCLUDE_DIR
                NAMES Microsoft.UI.Xaml.Markup.h
                PATHS ${WINDOWS_APP_SDK_PATHS}
                PATH_SUFFIXES include
            )

            find_library(MICROSOFT_WINDOWS_APP_SDK_LIBRARY
                NAMES Microsoft.WindowsAppSDK.lib
                PATHS ${WINDOWS_APP_SDK_PATHS}
                PATH_SUFFIXES lib
            )

            if(MICROSOFT_WINDOWS_APP_SDK_INCLUDE_DIR AND MICROSOFT_WINDOWS_APP_SDK_LIBRARY)
                set(Microsoft_WindowsAppSDK_FOUND TRUE)
                add_library(Microsoft::WindowsAppSDK INTERFACE IMPORTED)
                set_target_properties(Microsoft::WindowsAppSDK PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${MICROSOFT_WINDOWS_APP_SDK_INCLUDE_DIR}"
                    INTERFACE_LINK_LIBRARIES "${MICROSOFT_WINDOWS_APP_SDK_LIBRARY}"
                )
                message(STATUS "Found Windows App SDK: ${MICROSOFT_WINDOWS_APP_SDK_INCLUDE_DIR}")
            else()
                message(WARNING "Windows App SDK not found. Modern manager will not be built.")
                set(BUILD_MODERN_MANAGER OFF)
            endif()
        endif()
    endif()

    # Add MacType Manager subdirectory (Universal App)
    if(BUILD_MODERN_MANAGER)
        message(STATUS "Building MacType Manager as Universal WinUI 3 App (x64 + ARM64)")
        add_subdirectory(src/platform/manager)
    endif()
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
