cmake_minimum_required(VERSION 3.16)
project(fart VERSION 1.99.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Source files for refactored version
set(REFACTORED_SOURCES
    fart_refactored.cpp
    fart_config.hpp
    text_processor.cpp
    text_processor.hpp
    file_processor.cpp
    file_processor.hpp
    argument_parser.cpp
    argument_parser.hpp
)

# Source files for original version
set(ORIGINAL_SOURCES
    fart.cpp
    fart_shared.c
    fart_shared.h
    wildmat.c
)

# Refactored executable
add_executable(fart_refactored ${REFACTORED_SOURCES})
target_link_libraries(fart_refactored Threads::Threads)

# Original executable (for comparison)
add_executable(fart_original ${ORIGINAL_SOURCES})
target_link_libraries(fart_original Threads::Threads)

# Set C++ standard for original build
set_target_properties(fart_original PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

# Installation
install(TARGETS fart_refactored DESTINATION bin)
install(FILES README.md DESTINATION share/doc/fart)

# Testing
enable_testing()

# Basic functionality tests
add_test(NAME test_help COMMAND fart_refactored --help)
add_test(NAME test_version COMMAND fart_refactored --version)

# Create test directory and files
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data)
file(WRITE ${CMAKE_BINARY_DIR}/test_data/test.txt "hello world\ntest line\nhello again")

add_test(NAME test_find 
    COMMAND fart_refactored ${CMAKE_BINARY_DIR}/test_data/test.txt hello
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_test(NAME test_replace_preview
    COMMAND fart_refactored --preview ${CMAKE_BINARY_DIR}/test_data/test.txt hello hi
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Package configuration
set(CPACK_PACKAGE_NAME "fart")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "99")
set(CPACK_PACKAGE_VERSION_PATCH "4")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Find And Replace Text utility")
set(CPACK_PACKAGE_VENDOR "Lionello Lunesu")
set(CPACK_PACKAGE_CONTACT "https://github.com/lionello/fart-it")

include(CPack)