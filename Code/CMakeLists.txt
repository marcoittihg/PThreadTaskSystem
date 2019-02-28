cmake_minimum_required(VERSION 3.10)
project(Code)

set(CMAKE_CXX_STANDARD 17)

add_executable(Code PThreadPool.h PThreadPool.cpp TaskSystem.h TaskSystem.cpp TaskSystemUtility.h main.cpp)

add_executable(Testing Testing.cpp PThreadPool.h PThreadPool.cpp TaskSystem.h TaskSystem.cpp TaskSystemUtility.h)