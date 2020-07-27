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
add_subdirectory(igxi-tool)

# Setup test data

set(doShaderRecreate TRUE CACHE BOOL "Enable shader recompilation")
set(enableIgxTest TRUE CACHE BOOL "Enable IGX test")
file(GLOB_RECURSE shaders "res/shaders/*.comp" "res/shaders/*.vert" "res/shaders/*.frag" "res/test_shaders/*.comp" "res/test_shaders/*.vert" "res/test_shaders/*.frag")
file(GLOB_RECURSE shaderBinaries "res/shaders/*.spv")
file(GLOB_RECURSE shaderTestBinaries "res/test_shaders/*.spv")
file(GLOB_RECURSE fonts "res/fonts/*.ttf")

if(doShaderRecreate)
	if("$ENV{VULKAN_SDK}" STREQUAL "")
		message(FATAL_ERROR "Ignix requires the Vulkan SDK so it can compile the shaders with SPIR-V")
	endif()
endif()

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
target_include_directories(igx PUBLIC igxi-tool/include)
target_include_directories(igx PUBLIC igxi-tool/igxi/include)
target_include_directories(igx PUBLIC ignis/core2/include)
target_link_libraries(igx PUBLIC ignis ocore igxi igxi-convert)

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

# Add file dependencies ; the builtin shaders and fonts
	
add_virtual_files(
	DIRECTORY
		${CMAKE_CURRENT_SOURCE_DIR}/res/shaders
	NAME
		igx/shaders
	FILES
		${shaderBinaries}
)

add_virtual_files(
	DIRECTORY
		${CMAKE_CURRENT_SOURCE_DIR}/res/fonts
	NAME
		igx/fonts
	FILES
		${fonts}
)

#

if(enableIgxTest)
	
	add_executable(igx_test ${testsCpp})
	target_link_libraries(igx_test PUBLIC igx)

	add_virtual_files(
		DIRECTORY
			${CMAKE_CURRENT_SOURCE_DIR}/res/test_shaders
		NAME
			shaders
		FILES
			${shaderTestBinaries}
	)

	configure_icon(igx_test "${CMAKE_CURRENT_SOURCE_DIR}/res/icon.ico")
	configure_virtual_files(igx_test)
	
	if(doShaderRecreate)
		if(WIN32)
			add_custom_command(TARGET igx_test POST_BUILD COMMAND compile.bat ${shaders} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders")
		else()
			add_custom_command(TARGET igx_test POST_BUILD COMMAND compile.sh ${shaders} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders")
		endif()
	endif()

	if(MSVC)
		target_compile_options(igx_test PRIVATE /W4 /WX /MD /MP /wd4201 /EHsc /GR)
	else()
		target_compile_options(igx_test PRIVATE -Wall -Wextra -Werror -fms-extensions)
	endif()

endif()