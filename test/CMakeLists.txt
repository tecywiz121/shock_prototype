cmake_minimum_required(VERSION 2.8)
project(ShockTest CXX C)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -pedantic -std=c++0x -O0")
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/gtest)

enable_testing()

include_directories(${GTEST_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/../src/)
file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(shocktest ${TEST_SRC_FILES} ${PROJECT_SOURCE_DIR}/../src/IntParser.cpp)
add_dependencies(shocktest googletest)

target_link_libraries(shocktest ${GTEST_LIBS_DIR}/libgtest.a
                        ${GTEST_LIBS_DIR}/libgtest_main.a
                        pthread)

add_test(test1 shocktest)
