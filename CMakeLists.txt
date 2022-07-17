cmake_minimum_required(VERSION 3.23.2)

project(wav_lib LANGUAGES CXX)

find_package(fmt REQUIRED)

add_library(wav_lib INTERFACE)
target_include_directories(wav_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(wav_lib INTERFACE fmt::fmt)
target_compile_features(wav_lib INTERFACE cxx_std_20)

if(NOT WAV_LIB_NO_TEST)
    add_executable(test test.cpp)
    target_link_libraries(test PRIVATE wav_lib)
    target_compile_features(test PUBLIC cxx_std_20)
endif()
