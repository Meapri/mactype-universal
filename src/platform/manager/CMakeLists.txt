# CMakeLists.txt for MacType Manager (WinUI 3)

cmake_minimum_required(VERSION 3.20)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project name
project(MacTypeManager
    VERSION 1.0.0
    DESCRIPTION "Modern MacType Management Interface"
    LANGUAGES CXX)

# Set Windows SDK version
set(CMAKE_SYSTEM_VERSION 10.0.19041.0)

# Find Windows App SDK
find_package(Microsoft.WindowsAppSDK CONFIG REQUIRED)

# Source files
set(SOURCES
    "App.xaml.cpp"
    "MainWindow.xaml.cpp"
    "ModePage.xaml.cpp"
    "ProcessPage.xaml.cpp"
    "StylePage.xaml.cpp"
    "MacTypeCommunication.cpp"
    "pch.cpp"
)

set(HEADERS
    "pch.h"
    "App.xaml.h"
    "MainWindow.xaml.h"
    "ModePage.xaml.h"
    "ProcessPage.xaml.h"
    "StylePage.xaml.h"
    "MacTypeCommunication.h"
)

set(XAML_FILES
    "App.xaml"
    "MainWindow.xaml"
    "ModePage.xaml"
    "ProcessPage.xaml"
    "StylePage.xaml"
)

# Add executable
add_executable(MacTypeManager WIN32
    ${SOURCES}
    ${HEADERS}
    ${XAML_FILES}
)

# Precompile headers
target_precompile_headers(MacTypeManager PRIVATE pch.h)

# Link libraries
target_link_libraries(MacTypeManager
    PRIVATE
    Microsoft::WindowsAppSDK
)

# Include directories
target_include_directories(MacTypeManager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../agent
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

# Compiler options
target_compile_options(MacTypeManager PRIVATE
    /await
    /bigobj
    /permissive-
)

# Set Windows App SDK properties
set_target_properties(MacTypeManager PROPERTIES
    VS_GLOBAL_WindowsAppSDKBootstrapAutoInitializeOptions_Platform x64
    VS_GLOBAL_WindowsAppSDKBootstrapAutoInitializeOptions_Version 1.4
    VS_PACKAGE_REFERENCES "Microsoft.WindowsAppSDK_1.4"
)

# Add integration test executable
add_executable(MacTypeManagerTest
    test_integration.cpp
    MacTypeCommunication.cpp
    pch.cpp
)

target_precompile_headers(MacTypeManagerTest PRIVATE pch.h)

target_link_libraries(MacTypeManagerTest
    PRIVATE
    # Add any required libraries here
)

target_include_directories(MacTypeManagerTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../agent
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

# Installation
install(TARGETS MacTypeManager
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
