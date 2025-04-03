/**
  ******************************************************************************
  * @file           : stream.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/4/2
  ******************************************************************************
  */


#include "stream.h"

namespace sylar {

/// 保证读/写完整的 length 字节，即 阻塞式 读写，直到所有数据被处理完毕（或发生错误
int Stream::readFixSize(void *buffer, size_t length) {
    size_t offset = 0;
    int64_t left = length;  // left 记录剩余需要读取的字节数，初始值 length
    while(left > 0){        // 循环读取，直到读取完成
        int64_t len = read((char*)buffer + offset, left);
        if(len <= 0){       // 说明出错/断开连接，返回len
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int Stream::readFixSize(ByteArray::ptr ba, size_t length) {
    int64_t left = length;
    while(left > 0) {
        int64_t len = read(ba, left);
        if(len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}

int Stream::writeFixSize(const void* buffer, size_t length) {
    size_t offset = 0;
    int64_t left = length;
    while(left > 0) {
        int64_t len = write((const char*)buffer + offset, left);
        if(len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;

}

int Stream::writeFixSize(ByteArray::ptr ba, size_t length) {
    int64_t left = length;
    while(left > 0) {
        int64_t len = write(ba, left);
        if(len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}



}
