cmake_minimum_required(VERSION 3.0)
project(sumocv)

set(CMAKE_CXX_STANDARD 11)
set(PARROT_SDK_HOME $ENV{HOME}/Desktop/robot/parrot/sdk)

find_package(OpenCV 4.2.0 EXACT REQUIRED PATHS /home/sasha/Desktop/cv/opencv4.2.0-debug)

add_library(libsumo SHARED ${CMAKE_CURRENT_SOURCE_DIR}/../libsumo/JumpingSumo.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libsumo)
include_directories(${PARROT_SDK_HOME}/out/arsdk-native/staging/usr/include)
link_directories(${PARROT_SDK_HOME}/out/arsdk-native/staging/usr/lib)

add_executable(sumocv main.cpp)
target_link_libraries(sumocv libsumo arsal ardiscovery arcontroller ${OpenCV_LIBS})