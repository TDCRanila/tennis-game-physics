cmake_minimum_required(VERSION 3.17.2)

macro(SetCPPVersion Target)
set_property(TARGET ${Target} PROPERTY CXX_STANDARD 20)
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
elseif(MINGW)
    add_compile_options(-Wno-unknown-pragmas)
endif()

add_subdirectory(tennis)
