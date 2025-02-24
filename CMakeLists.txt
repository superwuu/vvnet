cmake_minimum_required(VERSION 3.10)
project(vvnet VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找并链接 pthread 库
find_package(Threads REQUIRED)

set(MAIN_FILE ./main.cc)
file(GLOB SRC "net/*.cc")
file(GLOB LOG_SRC "util/vvlog/*.cc")

set(SOURCES ${SRC} ${LOG_SRC} ${MAIN_FILE})

include_directories(
    ${PROJECT_SOURCE_DIR}/application
    ${PROJECT_SOURCE_DIR}/route
    ${PROJECT_SOURCE_DIR}/net
    ${PROJECT_SOURCE_DIR}/util
    ${PROJECT_SOURCE_DIR}
)

add_executable(vvnet ${SOURCES})

target_link_libraries(vvnet PRIVATE Threads::Threads)