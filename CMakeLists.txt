cmake_minimum_required(VERSION 3.0)
project(sylar)

#include (cmake/utils.cmake)
set(CMAKE_C_COMPILER /usr/bin/gcc)  # 替换为实际的 gcc 路径
set(CMAKE_CXX_COMPILER /usr/bin/g++)  # 替换为实际的 g++ 路径
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
#set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include (cmake/utils.cmake)

set(CMAKE_GENERATOR_PLATFORM x64)

include_directories(.)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)

set(LIB_SRC
        sylar/log.cpp
        sylar/util.cpp
        sylar/config.cpp
        sylar/thread.cpp
        sylar/mutex.cpp
        )

#生成共享库
add_library(sylar SHARED ${LIB_SRC})

set(LIBS
        sylar
        pthread
        yaml-cpp
)

#force_redefine_file_macro_for_sources(sylar)

add_executable(test tests/test.cpp)
add_dependencies(test sylar)
#force_redefine_file_macro_for_sources(test) #___FILE___
target_link_libraries(test  sylar "${LIBS}")

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
#force_redefine_file_macro_for_sources(test_config) #___FILE___
target_link_libraries(test_config  sylar "${LIBS}")

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread sylar)
#force_redefine_file_macro_for_sources(test_config) #___FILE___
target_link_libraries(test_thread sylar "${LIBS}")

#设置输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)