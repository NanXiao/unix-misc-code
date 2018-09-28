cmake_minimum_required(VERSION 3.10)
project(unix-misc)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads)

add_executable(thread-switch-pipe src/thread-switch-pipe.cpp)
add_executable(thread-switch-condvar src/thread-switch-condvar.cpp)

target_link_libraries(thread-switch-pipe ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(thread-switch-condvar ${CMAKE_THREAD_LIBS_INIT})
