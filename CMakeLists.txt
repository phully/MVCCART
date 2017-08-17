cmake_minimum_required(VERSION 2.8.4)
project(mvccart)


find_package(Boost 1.63.0 COMPONENTS  log log_setup system thread filesystem unit_test_framework REQUIRED)
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


OPTION(MVCC11_USES_STD_SHARED_PTR "Use std::shared_ptr instead of boost::shared_ptr" OFF)
OPTION(BUILD_MVCC11_TEST_WITH_STDLIB_LIBCPP "Build mvcc_test with -stdlib=libc++" OFF)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -Wall -std=c++1y")
set(SOURCE_FILES  ART/ARTFULCpp.h ART/ArtCPP.hpp Transactions/transactionManager.h mvcc/mvcc.hpp  WorkloadTests/ReadOnly.cpp)
add_subdirectory(fmt-master/fmt)
add_executable(mvccart ${SOURCE_FILES})
target_link_libraries(mvccart ${Boost_LIBRARIES} fmt::fmt-header-only)



