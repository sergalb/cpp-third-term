cmake_minimum_required(VERSION 3.0)

project(SubstringFinder)

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
  #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined")
endif()


add_executable(SubstringFinder main.cpp
    mainwindow.h mainwindow.cpp scanner.cpp scanner.h utf8_validator.hpp trigram.hpp)

set_target_properties(SubstringFinder PROPERTIES    CXX_STANDARD 17)

qt5_use_modules(SubstringFinder Concurrent)
qt5_use_modules(SubstringFinder Widgets)
