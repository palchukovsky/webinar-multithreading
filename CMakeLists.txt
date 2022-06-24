cmake_minimum_required(VERSION 3.5)
project(multithreading)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads)

add_executable(01_thread 01_thread.cpp)
target_link_libraries(01_thread
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(02_mutex 02_mutex.cpp)
target_link_libraries(02_mutex
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(03_promise 03_promise.cpp)
target_link_libraries(03_promise
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(04_async 04_async.cpp)
target_link_libraries(04_async
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(05_condition 05_condition.cpp)
target_link_libraries(05_condition
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(06_call_once 06_call_once.cpp)
target_link_libraries(06_call_once
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(07_packaged_task 07_packaged_task.cpp)
target_link_libraries(07_packaged_task
  ${CMAKE_THREAD_LIBS_INIT}
)
