cmake_minimum_required(VERSION 3.9)
project(Code)

set(CMAKE_CXX_STANDARD 11)

add_executable(Code main.cpp PThreadPool.h PThreadPool.cpp TaskSystem.h TaskSystem.cpp TaskSystemUtility.h)

add_executable(Testing Testing.cpp PThreadPool.h PThreadPool.cpp TaskSystem.h TaskSystem.cpp TaskSystemUtility.h)