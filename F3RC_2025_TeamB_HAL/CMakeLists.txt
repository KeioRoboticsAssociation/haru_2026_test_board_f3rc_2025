cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME F3RC_TEAMB_HAL)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME} C CXX ASM)
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
file(GLOB_RECURSE USER_SOURCES Src/*.c Src/*.cpp)

# Enabled libraries under Lib/ (semicolon-separated), or "ALL" for everything.
set(F446_LIBS "ALL" CACHE STRING "Lib modules under Lib/ to build (semicolon-separated), or ALL")
set(USER_LIB_SOURCES "")
if(F446_LIBS STREQUAL "ALL")
    file(GLOB_RECURSE USER_LIB_SOURCES CONFIGURE_DEPENDS Lib/*/Src/*.c Lib/*/Src/*.cpp)
else()
    foreach(lib ${F446_LIBS})
        file(GLOB_RECURSE ONE_LIB_SOURCES CONFIGURE_DEPENDS "Lib/${lib}/Src/*.c" "Lib/${lib}/Src/*.cpp")
        list(APPEND USER_LIB_SOURCES ${ONE_LIB_SOURCES})
    endforeach()
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${USER_SOURCES}
    ${USER_LIB_SOURCES}
)

# Add include paths
file(GLOB USER_LIB_DIRS Lib/*/Inc)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Inc
    ${USER_LIB_DIRS}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
