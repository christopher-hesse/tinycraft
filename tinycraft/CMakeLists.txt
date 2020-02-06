cmake_minimum_required(VERSION 3.11)
project(voxel VERSION 1.0.1 DESCRIPTION "voxel environment")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

option(LINUX_HEADLESS_SOFTWARE_RENDERING "Set if we are on linux and want to use software rendering" OFF)
option(LINUX_HEADLESS_HARDWARE_RENDERING "Set if we are on linux and want to use hardware rendering" OFF)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third-party)

if(MSVC)
  # don't warn for using getenv
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  # use secure functions for MSVC
  add_definitions(-DSTBI_MSC_SECURE_CRT)
endif()

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
if(LINUX_HEADLESS_SOFTWARE_RENDERING)
  set(GLFW_USE_OSMESA ON CACHE BOOL "" FORCE)
elseif(LINUX_HEADLESS_HARDWARE_RENDERING)
  set(GLFW_USE_EGLHEADLESS ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(${THIRD_PARTY_DIR}/glfw-headless glfw)
list(APPEND LIBS glfw)

# glad
add_library(glad ${THIRD_PARTY_DIR}/glad-gl-3.3-core-anisotropy/src/glad.c)
target_include_directories(glad PUBLIC ${THIRD_PARTY_DIR}/glad-gl-3.3-core-anisotropy/include)
list(APPEND LIBS glad)

# glm
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${THIRD_PARTY_DIR}/glm-0.9.9.6)
list(APPEND LIBS glm)

# stb
include_directories(SYSTEM ${THIRD_PARTY_DIR}/stb)

# libenv
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# create shared library

add_library(shared_library SHARED
  src/unity_shared_library.cpp
)
target_link_libraries(shared_library ${LIBS})
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # these compiler options are for gcc, so only apply them when we are using that compiler, which will happen
  # when we are running tests
  target_compile_options(shared_library PRIVATE -Wall -Werror -Wextra -Wshadow -Wpedantic -Wformat=2 -Wundef -Wvla -Wmissing-include-dirs -Wnon-virtual-dtor)
endif()

add_executable(human
  src/unity_human.cpp
)
target_link_libraries(human ${LIBS})

# test
# based on https://github.com/google/googletest/blob/master/googletest/README.md#incorporating-into-an-existing-cmake-project
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${THIRD_PARTY_DIR}/googletest-release-1.8.1 ${CMAKE_CURRENT_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)
add_executable(tests src/unity_test.cpp)
target_link_libraries(tests gtest gtest_main ${LIBS})

# benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
add_subdirectory(${THIRD_PARTY_DIR}/benchmark-1.4.1 ${CMAKE_CURRENT_BINARY_DIR}/benchmark)
include_directories(${THIRD_PARTY_DIR}/benchmark-1.4.1/include)
add_executable(benchmarks src/unity_benchmark.cpp)
target_link_libraries(benchmarks benchmark benchmark_main ${LIBS})