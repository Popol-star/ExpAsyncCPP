cmake_minimum_required(VERSION 3.5.0)
project(ExpAsyncCPP VERSION 0.1.0 LANGUAGES C CXX)
set(CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED TRUE)
add_library(ExpAsyncCPP src/BlockingAsyncExecutor.cpp)

