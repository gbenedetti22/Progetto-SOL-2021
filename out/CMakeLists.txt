cmake_minimum_required(VERSION 3.19)
project(Progetto C)

set(CMAKE_C_STANDARD 99)

add_executable(server ../server.c)
add_executable(client ../client.c)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(server Threads::Threads)