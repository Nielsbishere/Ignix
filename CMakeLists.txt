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

file(GLOB_RECURSE depsHpp "third_party/include/*.hpp")

file(GLOB_RECURSE testsCpp "test/*.cpp")

add_subdirectory(ignis)
add_subdirectory(igxi)

# Setup test data

file(GLOB_RECURSE shaders "res/shaders/*")

# Nuklear dependency

file(GLOB_RECURSE nuklear "third_party/Nuklear/src/*.h" "third_party/Nuklear/src/*.c")

add_library(nuklear STATIC ${nuklear})

add_custom_command(TARGET nuklear POST_BUILD COMMAND paq WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Nuklear/src)

# Link library

add_executable(
	ignix
	${ignixHpp}
	${ignixCpp}
	${depsHpp}
	${shaders}
	${testsCpp}
	CMakeLists.txt
	third_party/Nuklear/nuklear.h	# Add the output of nuklear
)

set_property(TARGET ignix PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/res")

target_include_directories(ignix PRIVATE include)
target_include_directories(ignix PRIVATE third_party/include)
target_include_directories(ignix PRIVATE Ignis/include)
target_include_directories(ignix PRIVATE Ignis/core2/include)
target_link_libraries(ignix PRIVATE nuklear ignis ocore igxi)

source_group("Headers" FILES ${ignisHpp})
source_group("Source" FILES ${ignisCpp})
source_group("Deps headers" FILES ${depsHpp})
source_group("Deps source" FILES ${depsCpp})
source_group("Shaders" FILES ${shaders})

if(MSVC)
    target_compile_options(ignix PRIVATE /W4 /WX /MD /MP /wd26812)
else()
    target_compile_options(ignix PRIVATE -Wall -Wextra -pedantic -Werror)
endif()