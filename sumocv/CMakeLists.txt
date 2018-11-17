cmake_minimum_required(VERSION 3.0)
project(sumocv)

set(CMAKE_CXX_STANDARD 11)
set(PARROT_SDK_HOME $ENV{HOME}/Desktop/robot/parrot/sdk)

find_package(OpenCV REQUIRED)

include_directories(${PARROT_SDK_HOME}/out/arsdk-native/staging/usr/include)
link_directories(${PARROT_SDK_HOME}/out/arsdk-native/staging/usr/lib)

add_executable(sumocv main.cpp JumpingSumo.cpp JumpingSumo.h)
target_link_libraries(sumocv arsal ardiscovery arcontroller ${OpenCV_LIBS})