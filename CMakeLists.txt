cmake_minimum_required(VERSION 3.13.0)
project(igx)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

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

set(enableIgxTest TRUE CACHE BOOL "Enable IGX test")
file(GLOB_RECURSE shaders "res/shaders/*.comp" "res/shaders/*.vert" "res/shaders/*.frag")
file(GLOB_RECURSE shaderBinaries "res/shaders/*.spv")

# Nuklear dependency

set(enableNuklearBuild FALSE CACHE BOOL "Enable Nuklear header generate")

if(enableNuklearBuild)

	file(GLOB_RECURSE nuklear "third_party/Nuklear/src/*.h" "third_party/Nuklear/src/*.c")

	add_library(nuklear STATIC ${nuklear})

	if(WIN32)
		add_custom_command(TARGET nuklear POST_BUILD COMMAND paq WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Nuklear/src)
	else()
		add_custom_command(TARGET nuklear POST_BUILD COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Nuklear/src/paq.sh" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Nuklear/src)
	endif()
endif()

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

target_include_directories(igx PUBLIC include)
target_include_directories(igx PRIVATE third_party)
target_include_directories(igx PUBLIC ignis/include)
target_include_directories(igx PUBLIC ignis/core2/include)
	target_link_libraries(igx PRIVATE igxi)
target_link_libraries(igx PUBLIC ignis ocore)

if(enableNuklearBuild)
	target_link_libraries(igx PRIVATE nuklear)
endif()

source_group("Headers" FILES ${ignisHpp})
source_group("Source" FILES ${ignisCpp})
source_group("Deps headers" FILES ${depsHpp} third_party/Nuklear/nuklear.h)
source_group("Deps source" FILES ${depsCpp})
source_group("Shaders" FILES ${shaders})
source_group("Shader binaries" FILES ${shaderBinaries})

if(MSVC)
    target_compile_options(igx PRIVATE /W4 /WX /MD /MP /wd26812 /wd4201 /EHsc /GR)
else()
    target_compile_options(igx PRIVATE -Wall -fms-extensions -Wextra -Werror)
endif()

if(enableIgxTest)

	add_executable(igx_test ${testsCpp})
	target_link_libraries(igx_test PUBLIC igx)

	add_custom_command(TARGET igx_test POST_BUILD COMMAND compile.sh ${shaders} WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/res/shaders")
	set_property(TARGET igx_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/res")

	if(MSVC)
		target_compile_options(igx_test PRIVATE /W4 /WX /MD /MP /wd4201 /EHsc /GR)
	else()
		target_compile_options(igx_test PRIVATE -Wall -Wextra -Werror -fms-extensions)
	endif()

endif()