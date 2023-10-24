cmake_minimum_required(VERSION 3.10)
project(FileSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(FileSystem
	FileSystem/Controller.cpp
	FileSystem/Inode.cpp
	FileSystem/Source.cpp
	FileSystem/Superblock.cpp
	FileSystem/UI.cpp)

if(MSVC)
	target_compile_options(FileSystem PRIVATE /W4)
else()
	target_compile_options(FileSystem PRIVATE -Wall -Wextra -Wpedantic)
endif()
