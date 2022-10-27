cmake_minimum_required(VERSION 3.0)
project(chat)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置最终的可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件的搜索路径
include_directories(/home/xl22/桌面/chat/include)
include_directories(/home/xl22/桌面/chat/include/server)
include_directories(/home/xl22/桌面/chat/include/server/db)
include_directories(/home/xl22/桌面/chat/include/server/model)
include_directories(/home/xl22/桌面/chat/include/server/redis)
include_directories(/home/xl22/桌面/chat/thirdparty)
link_directories(/usr/lib/mysql)

# 加载子目录
add_subdirectory(src)