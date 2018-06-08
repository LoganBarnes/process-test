cmake_minimum_required(VERSION 3.10)
project(Pipes)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost 1.65 REQUIRED COMPONENTS filesystem)

add_executable(Pipes main.cpp)
target_link_libraries(Pipes Threads::Threads)
target_link_libraries(Pipes Boost::filesystem)

add_executable(Pipes2 main2.cpp)
target_link_libraries(Pipes2 Threads::Threads)
target_link_libraries(Pipes2 Boost::filesystem)
