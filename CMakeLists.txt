# CMake entry point
cmake_minimum_required (VERSION 3.18)

set( PROJECT_NAME sample-ns )
project( ${PROJECT_NAME} )

add_executable( ${PROJECT_NAME}
	src/main.c
)

