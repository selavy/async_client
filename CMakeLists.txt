cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(async_client C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost 1.55.0 REQUIRED COMPONENTS log log_setup filesystem thread date_time program_options system)
include_directories(${Boost_INCLUDE_DIRS})
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
set(RAPIDJSON_USE_SSE2 ON)
set(RAPIDJSON_USE_SSE42 ON)
set(RAPIDJSON_DIR "${CMAKE_SOURCE_DIR}/rapidjson")
get_filename_component(RAPIDJSON_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(RAPIDJSON_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/src/rapidjson")
include_directories("${RAPIDJSON_INCLUDE_DIRS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RAPIDJSON_CXX_FLAGS}")

add_compile_options(-Wall -Werror)
add_compile_options(--std=c++11)

add_subdirectory(src)
