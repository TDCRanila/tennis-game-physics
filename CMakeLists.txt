cmake_minimum_required(VERSION 3.25)

include(cmake/utility.cmake)

set(CMAKE_COLOR_MAKEFILE ON)

if (WINDOWS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build-output-win64")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build-output")
endif()

macro(SetCPPVersion Target)
    set_property(TARGET ${Target} PROPERTY CXX_STANDARD 23)
    set_property(TARGET ${Target} PROPERTY CXX_STANDARD_REQUIRED ON)
endmacro()

### Project.
set(PROJECT_NAME Tennis)
project(${PROJECT_NAME})

### Project Properties.
if (MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_PredfinedTargetsFolder")
    
    #add_compile_definitions(/MP)
    add_link_options(/INCREMENTAL:NO)

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++2c")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2c")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-changes-meaning")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++26-extensions")

endif()

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(tennis)
