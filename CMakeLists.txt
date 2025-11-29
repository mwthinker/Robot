cmake_minimum_required(VERSION 4.2)
project(Robot
	DESCRIPTION
		"Description"
	VERSION
		0.1.0
	LANGUAGES
		CXX
)

# Copy data to build folder.
file(COPY data/. DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(Robot
	src/camera.cpp
	src/camera.h
	src/graphic.h
	src/main.cpp
	src/robotgraphics.h
	src/robotgraphics.cpp
	src/robotwindow.cpp
	src/robotwindow.h
	src/sphereviewvar.h
	src/sphereviewvar.cpp
	
	CMakePresets.json
	vcpkg.json
	cppsdl3.natvis
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Robot)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Robot_Test)

find_package(cppsdl3 CONFIG REQUIRED)


if (MSVC)
	target_compile_options(Robot
		PUBLIC
			/W3 /WX /permissive-
			/MP
	)
else ()
	target_compile_options(Robot
		PRIVATE
			-Wall -pedantic -Wcast-align -Woverloaded-virtual -Wno-unused-parameter -Wno-sign-compare -Wno-unused-function
	)
endif ()

target_link_libraries(Robot
	PRIVATE
		CppSdl3::CppSdl3
)

set_target_properties(Robot
	PROPERTIES
		CXX_STANDARD 23
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
)
