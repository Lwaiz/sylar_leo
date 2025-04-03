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

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
        include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(LIB_SRC
        sylar/log.cpp
        sylar/util.cpp
        sylar/config.cpp
        sylar/thread.cpp
        sylar/mutex.cpp
        sylar/macro.cpp
        sylar/fiber.cpp
        sylar/scheduler.cpp
        sylar/iomanager.cpp
        sylar/timer.cpp
        sylar/hook.cpp
        sylar/fd_manager.cpp
        sylar/address.cpp
        sylar/socket.cpp
        sylar/bytearray.cpp
        sylar/http/http.cpp
        sylar/http/http11_parser.cpp
        sylar/http/httpclient_parser.cpp
        sylar/http/http_parser.cpp
        sylar/tcp_server.cpp
        sylar/stream.cpp
        sylar/streams/socket_stream.cpp
        sylar/http/http_session.cpp
        sylar/http/http_server.cpp
        sylar/http/servlet.cpp
        sylar/http/http_connection.cpp
        )



#生成共享库
add_library(sylar SHARED ${LIB_SRC})

set(LIBS
        sylar
        dl
        pthread
        yaml-cpp
        ${ZLIB_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
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
target_link_libraries(test_thread sylar "${LIBS}")

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util sylar)
target_link_libraries(test_util sylar "${LIBS}")

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber sylar)
target_link_libraries(test_fiber sylar "${LIBS}")

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler sylar "${LIBS}")

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager sylar)
target_link_libraries(test_iomanager sylar "${LIBS}")

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook sylar)
target_link_libraries(test_hook sylar "${LIBS}")

add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address sylar)
target_link_libraries(test_address sylar "${LIBS}")

add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket sylar)
target_link_libraries(test_socket sylar "${LIBS}")

add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray sylar)
target_link_libraries(test_bytearray sylar "${LIBS}")

add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http sylar)
target_link_libraries(test_http sylar "${LIBS}")

add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser sylar)
target_link_libraries(test_http_parser sylar "${LIBS}")

add_executable(test_tcp_server tests/test_tcp_server.cpp)
add_dependencies(test_tcp_server sylar)
target_link_libraries(test_tcp_server sylar "${LIBS}")












#设置输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)