cmake_minimum_required(VERSION 3.19)
project(Progetto C)

set(CMAKE_C_STANDARD 99)
file(GLOB server_lib CONFIGURE_DEPENDS "../lib/src/*.c")
file(GLOB client_lib CONFIGURE_DEPENDS "../lib/src/*.c")

list(FILTER server_lib EXCLUDE REGEX "../lib/src/fs_client_api.c")

add_executable(server ../server.c ${server_lib})
add_executable(client ../client.c ${client_lib})
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(server Threads::Threads)
target_link_libraries(client Threads::Threads)