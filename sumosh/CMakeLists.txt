cmake_minimum_required(VERSION 3.0)
project(sumosh)

set(CMAKE_CXX_STANDARD 11)
set(PARROT_SDK_HOME $ENV{HOME}/Desktop/robot/parrot/sdk)

include_directories(${PARROT_SDK_HOME}/out/arsdk-native/staging/usr/include)
link_directories(${PARROT_SDK_HOME}/out/arsdk-native/staging/usr/lib)

add_executable(sumosh main.cpp JumpingSumo.cpp JumpingSumo.h)
target_link_libraries(sumosh arsal ardiscovery arcontroller)