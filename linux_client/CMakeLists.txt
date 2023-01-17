cmake_minimum_required(VERSION 3.14)
project(s_clipboard)

set(NOTIFY on)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../socket-cpp socket-cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../json/ json)

include_directories(../socket-cpp/Socket)
include_directories(../json/include)

add_executable(${PROJECT_NAME} main.cpp server.cpp server.h client.cpp client.h clipboard.cpp clipboard.h)
target_link_libraries(${PROJECT_NAME} pthread socket X11)

if(NOTIFY)
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(deps REQUIRED IMPORTED_TARGET gio-2.0)
    add_definitions(-DNOTIFY)
		target_link_libraries(${PROJECT_NAME} PkgConfig::deps)
endif()
