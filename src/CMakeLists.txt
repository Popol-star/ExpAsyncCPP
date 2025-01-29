cmake_minimum_required(VERSION 3.5.0)
add_library(ExpAsyncCPP "AsyncExecutor.cpp")
target_include_directories(ExpAsyncCPP
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
  )