cmake_minimum_required(VERSION 3.15)
project(MyLibrary VERSION 1.0.0 LANGUAGES CXX)

# Standard settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json (handy for editors/tools)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# On some platforms you must explicitly request PIC for shared libs
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# 2. Find the pybind11 package installed via pip
find_package(pybind11 CONFIG REQUIRED)

# --- Library target ---
# add_library(display-controller SHARED
#     src/OLED_SSD1306.cpp
# )

# target_link_libraries(display_controller PRIVATE Python3::Python)

pybind11_add_module(display_controller src/OLED_SSD1306.cpp)

# Public headers used by consumers of the library
target_include_directories(display_controller
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Set shared-library version info (SONAME etc.)
set_target_properties(display_controller PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    # Symbol visibility: hide everything by default
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
if(NOT NO_TESTS)
    find_package(GTest REQUIRED)

    enable_testing()
    add_executable(run_tests tests/tests.cpp src/OLED_SSD1306.cpp)
    target_link_libraries(run_tests GTest::gtest GTest::gtest_main Python3::Python)
    # target_link_libraries(run_tests PRIVATE Python3::Python)
    target_include_directories(run_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    # Enable CTest discovery
    include(GoogleTest)
    gtest_discover_tests(run_tests)
endif()

# Any dependencies
# find_package(fmt REQUIRED)
# target_link_libraries(mylib PRIVATE fmt::fmt)

# --- Example executable using the library ---
# add_executable(myapp app/main.cpp)
# target_link_libraries(myapp PRIVATE mylib)

# --- Install rules ---
# include(GNUInstallDirs)

# install(TARGETS mylib
#     EXPORT mylibTargets
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # needed for .dll on Windows
# )

# install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/mylib/export.h
#         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mylib)

# install(EXPORT mylibTargets
#     FILE mylibTargets.cmake
#     NAMESPACE mylib::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mylib
# )
