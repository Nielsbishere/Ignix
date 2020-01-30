cmake_minimum_required(VERSION 3.13.0)
project(igx)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE igxHpp "include/*.hpp")
file(GLOB_RECURSE igxCpp "src/*.cpp")

file(GLOB_RECURSE depsHpp "third_party/include/*.hpp")
file(GLOB_RECURSE depsCpp "third_party/src/*.cpp")

file(GLOB_RECURSE testsCpp "test/*.cpp")

add_subdirectory(ignis)
add_subdirectory(igxi)

# Setup test data

file(GLOB_RECURSE shaders "res/shaders/*.comp" "res/shaders/*.vert" "res/shaders/*.frag")
file(GLOB_RECURSE shaderBinaries "res/shaders/*.spv")

# Nuklear dependency

file(GLOB_RECURSE nuklear "third_party/Nuklear/src/*.h" "third_party/Nuklear/src/*.c")

add_library(nuklear STATIC ${nuklear})

# add_custom_command(TARGET nuklear POST_BUILD COMMAND paq WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Nuklear/src)

# Link library

add_library(
	igx STATIC
	${igxHpp}
	${igxCpp}
	${depsHpp}
	${depsCpp}
	${shaders}
	${shaderBinaries}
	CMakeLists.txt
	third_party/Nuklear/nuklear.h	# Add the output of nuklear
)

target_include_directories(igx PRIVATE include)
target_include_directories(igx PRIVATE third_party)
target_include_directories(igx PRIVATE Ignis/include)
target_include_directories(igx PRIVATE Ignis/core2/include)
target_link_libraries(igx PRIVATE nuklear ignis ocore igxi)

source_group("Headers" FILES ${ignisHpp})
source_group("Source" FILES ${ignisCpp})
source_group("Deps headers" FILES ${depsHpp} third_party/Nuklear/nuklear.h)
source_group("Deps source" FILES ${depsCpp})
source_group("Shaders" FILES ${shaders})
source_group("Shader binaries" FILES ${shaderBinaries})

if(MSVC)
    target_compile_options(igx PRIVATE /W4 /WX /MD /MP /wd26812 /wd4201 /EHsc /GR)
else()
    target_compile_options(igx PRIVATE -Wall -Wpedantic -Wextra -Werror)
endif()