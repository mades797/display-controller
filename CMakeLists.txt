cmake_minimum_required(VERSION 3.15)
project(MyLibrary VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

# --- Core logic library (testable, no pybind11 dependency) ---
add_library(display_controller_core STATIC
    src/OLED_SSD1306.cpp
)

target_include_directories(display_controller_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- Python bindings module (thin wrapper, no logic of its own) ---
pybind11_add_module(display_controller src/bindings.cpp)
target_link_libraries(display_controller PRIVATE display_controller_core)

set_target_properties(display_controller PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# --- Tests ---
option(NO_TESTS "Skip building tests" OFF)
if(NOT NO_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    # Keep gtest's own runtime settings consistent with this project on Windows/MSVC
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(display_controller_tests
        tests/tests.cpp
    )

    target_link_libraries(display_controller_tests
        PRIVATE
            display_controller_core
            GTest::gtest_main
            GTest::gmock_main
    )

    include(GoogleTest)
    gtest_discover_tests(display_controller_tests)
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
