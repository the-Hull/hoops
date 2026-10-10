cmake_minimum_required(VERSION 3.21)
project(hoops-host-tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

include(FetchContent)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        52eb8108c5bdec04579160ae17225d66034bd723)
FetchContent_MakeAvailable(googletest)

set(LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs")
file(GLOB SCORE_SOURCES CONFIGURE_DEPENDS "${LIBS}/score/*.cpp")
file(GLOB TEST_SOURCES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/score/*_test.cpp")

add_executable(hoops-host-tests ${SCORE_SOURCES} ${TEST_SOURCES})
target_include_directories(hoops-host-tests PRIVATE "${LIBS}")
target_link_libraries(hoops-host-tests PRIVATE GTest::gtest_main)
enable_testing()
include(GoogleTest)
gtest_discover_tests(hoops-host-tests)

# Host tool for the self-test's phase 3: a full index + 50 session files + 10 user workouts.
add_executable(hoops-seed-index ${SCORE_SOURCES} "${CMAKE_CURRENT_LIST_DIR}/tools/seed_index.cpp")
target_include_directories(hoops-seed-index PRIVATE "${LIBS}")
