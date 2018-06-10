cmake_minimum_required(VERSION 3.10)
project(Pipes)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost 1.65 REQUIRED COMPONENTS filesystem regex)

add_executable(Pipes main.cpp)
target_link_libraries(Pipes Threads::Threads)
target_link_libraries(Pipes Boost::filesystem)
target_link_libraries(Pipes Boost::regex)

add_executable(Pipes2 raw.cpp)
target_link_libraries(Pipes2 Threads::Threads)
target_link_libraries(Pipes2 Boost::filesystem)
target_link_libraries(Pipes2 Boost::regex)

add_executable(Pipes3 main3.cpp)
target_link_libraries(Pipes3 Threads::Threads)
target_link_libraries(Pipes3 Boost::filesystem)
target_link_libraries(Pipes3 Boost::regex)

add_executable(SO minimal.cpp)
