cmake_minimum_required(VERSION 3.0.0)
project(mixer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -lstdc++fs -pthread")

link_libraries(stdc++fs)

find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
find_package(OpenAL REQUIRED)
find_package(SndFile CONFIG REQUIRED)

add_executable(mixer 
    src/main.cpp
    src/SamplePlayer.cpp
    src/SamplePlayer.h
    src/SoundDevice.cpp
    src/SoundDevice.h
)

target_link_libraries(mixer
    "${OPENAL_LIBRARY}" 
    SndFile::sndfile
    ${WIRINGPI_LIBRARIES}
)

target_include_directories(mixer PUBLIC "${OPENAL_INCLUDE_DIR}")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})