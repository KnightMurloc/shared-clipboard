cmake_minimum_required(VERSION 3.14)
project(s_clipboard)

set(NOTIFY on)


find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK4 REQUIRED gtk4)
INCLUDE_DIRECTORIES(${GTK4_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK4_LIBRARY_DIRS})
ADD_DEFINITIONS(${GTK4_CFLAGS_OTHER})

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../socket-cpp socket-cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../json/ json)

if(NOTIFY)
    find_library(NOTIFY_LIBRARY notify)
    add_definitions(-DNOTIFY)
endif()

include_directories(../socket-cpp/Socket)
include_directories(../json/include)

add_executable(${PROJECT_NAME} main.cpp server.cpp server.h client.cpp client.h)
target_link_libraries(${PROJECT_NAME} ${GTK4_LIBRARIES} pthread socket ${NOTIFY_LIBRARY})

if(NOTIFY)
	target_link_libraries(${PROJECT_NAME} ${NOTIFY_LIBRARY})
endif()
