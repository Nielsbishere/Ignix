cmake_minimum_required(VERSION 3.13.0)
project(ignix)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE ignixHpp "include/*.hpp")
file(GLOB_RECURSE ignixCpp "src/*.cpp")

add_subdirectory(ignis)
add_subdirectory(igxi)

# Setup test data

file(GLOB_RECURSE shaders "res/shaders/*")

# Link library

add_executable(
	ignix
	${ignixHpp}
	${ignixCpp}
	${shaders}
	CMakeLists.txt
	test/test.cpp
)

set_property(TARGET ignix PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/res")

target_include_directories(ignix PRIVATE include)
target_include_directories(ignix PRIVATE Ignis/include)
target_include_directories(ignix PRIVATE Ignis/core2/include)
target_link_libraries(ignix PRIVATE ignis ocore igxi)

source_group("Headers" FILES ${ignisHpp})
source_group("Source" FILES ${ignisCpp})
source_group("Shaders" FILES ${shaders})

if(MSVC)
    target_compile_options(ignix PRIVATE /W4 /WX /MD /MP /wd26812)
else()
    target_compile_options(ignix PRIVATE -Wall -Wextra -pedantic -Werror)
endif()