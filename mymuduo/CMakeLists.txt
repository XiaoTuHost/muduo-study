cmake_minimum_required(VERSION 2.8)

project(mymuduo)

# linux编译动态库so
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 编译信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

# 搜索 源文件变量
aux_source_directory(. SRC_LIST)

# 生成动态库
add_library(mymuduo SHARED ${SRC_LIST})