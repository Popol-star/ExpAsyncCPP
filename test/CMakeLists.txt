cmake_minimum_required(VERSION 3.5.0)
add_executable(AsyncTest test.cpp)
target_link_libraries(AsyncTest PUBLIC ExpAsyncCPP)