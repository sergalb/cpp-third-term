cmake_minimum_required(VERSION 3.0)

project(Copyfinder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -O0  -pedantic")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined")
endif()

add_library(xxhash STATIC xxhash_cpp/xxhash/xxhash.h xxhash_cpp/xxhash/xxhash.cpp xxhash_cpp/xxhash/xxhash.hpp)


add_executable(Copyfinder main.cpp
    mainwindow.h mainwindow.cpp equals_class.h equals_class.cpp scanner.cpp scanner.h openfileexception.h)

target_link_libraries(Copyfinder xxhash -lpthread)

set_target_properties(Copyfinder PROPERTIES    CXX_STANDARD 17)

qt5_use_modules(Copyfinder Concurrent)
qt5_use_modules(Copyfinder Widgets)
