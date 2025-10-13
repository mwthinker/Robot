project(Robot_Test
	DESCRIPTION
		"Testing project using GTest"
	LANGUAGES
		CXX
)

find_package(GTest CONFIG REQUIRED)
enable_testing()

add_executable(Robot_Test
    src/tests.cpp
    

    CMakeLists.txt
)

target_link_libraries(Robot_Test
    PUBLIC
        GTest::gtest GTest::gtest_main # Test explorer on Visual Studio 2022 will not find test if "GTest::gmock_main GTest::gmock" is added?
)

if (MSVC)
    target_compile_options(Robot_Test
        PRIVATE
            "/permissive-"
    )
endif ()

set_target_properties(Robot_Test
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

include(GoogleTest)
gtest_discover_tests(Robot_Test)
