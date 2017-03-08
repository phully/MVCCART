cmake_minimum_required(VERSION 2.8.4)
project(mvccart)


find_package(Boost 1.63.0 COMPONENTS system filesystem REQUIRED)
add_definitions(-DBOOST_LOG_DYN_LINK)

if(Boost_FOUND)

    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
    link_directories(SYSTEM ${Boost_LIBRARY_DIRS})

    set(Boost_USE_STATIC_LIBS        ON) # only find static libs
    set(Boost_USE_MULTITHREADED      ON)
    set(Boost_USE_STATIC_RUNTIME    OFF)
    set(BOOST_ALL_DYN_LINK           ON)   # force dynamic linking for all libraries
endif()

include_directories(core)
include_directories(core/parser)
include_directories(libcpp)
include_directories(libcpp/types)
set(CMAKE_CXX_STANDARD 11 )
set(SOURCE_FILES main.cpp art.c art.h AdaptiveRadixTreeTable.h table/BaseTable.hpp table/Table.hpp table/TableException.hpp  table/TableInfo.hpp table/TableInfo.cpp )
add_executable(mvccart ${SOURCE_FILES})
target_link_libraries (mvccart ${Boost_LIBRARIES})