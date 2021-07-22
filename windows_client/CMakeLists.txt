cmake_minimum_required(VERSION 3.18)
project(s_clipboard)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../socket-cpp socket-cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../json/ json)

include_directories(../socket-cpp/Socket)
include_directories(../json/include)

set(CMAKE_CXX_FLAGS "-mwindows")

add_executable(${PROJECT_NAME} main.cpp server.cpp server.h client.cpp client.h)
target_link_libraries(${PROJECT_NAME} socket nlohmann_json)