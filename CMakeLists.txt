cmake_minimum_required(VERSION 3.6)
project(mvccart)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp art.c art.h BaseTable.hpp HashMapTable.hpp RDBTable.hpp Table.hpp TableException.hpp TableInfo.cpp TableInfo.hpp)


add_executable(mvccart ${SOURCE_FILES})