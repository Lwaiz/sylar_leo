/**
  ******************************************************************************
  * @file           : bytearray.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/25
  ******************************************************************************
  */


#include "bytearray.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>

#include "endian.h"
#include "log.h"


namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


ByteArray::Node::Node(size_t s)
    :ptr(new char[s])  // 为 ptr 分配大小为 s 的内存
    ,next(nullptr)     // next 指针初始化为 nullptr
    ,size(s)  {        // 设置 size 为传入的 s
}

ByteArray::Node::Node()
    :ptr(nullptr)
    ,next(nullptr)
    ,size(0){   // 不分配内存 创建空节点
}

ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;   // delete[]  释放 ptr 指向的数组内存区域
    }
}


ByteArray::ByteArray(size_t base_size)
    :m_baseSize(base_size)          // 设置基本大小
    ,m_position(0)                  // 设置当前读取位置为 0
    ,m_capacity(base_size)          // 初始化容量为基本大小
    ,m_size(0)                      // 初始化实际大小为 0
    ,m_endian(SYLAR_BIG_ENDIAN)     // 设置字节序为大端
    ,m_root(new Node(base_size)) // 创建根节点并分配内存
    ,m_cur(m_root) {                // 当前节点指向根节点
}

ByteArray::~ByteArray(){
    Node* tmp = m_root;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

bool ByteArray::isLittleEndian() const {
    return m_endian == SYLAR_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = SYLAR_LITTLE_ENDIAN;
    } else {
        m_endian = SYLAR_BIG_ENDIAN;
    }
}

/// write

void ByteArray::writeFint8 (int8_t value){
    write(&value, sizeof(value));  // 直接写入 8 位有符号整数
}

void ByteArray::writeFuint8 (uint8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFint16 (int16_t value){
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);          // 如果字节序不同 转换字节序
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint16 (uint16_t value){
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint32 (int32_t value){
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint32 (uint32_t value){
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint64 (int64_t value){
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint64 (uint64_t value){
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief Zigzag 编码  负数被转换成更大的正数，而正数则被转换成较小的值
 * @param v
 * @return
 */
static uint32_t EncodeZigzag32(const int32_t& v) {
    if( v < 0 ){
        // 对于负数，取其绝对值，并将其乘以 2 然后减去 1，这样负数被映射到一个奇数值
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        // 对于正数，将其乘以 2
        return v * 2;
    }
}
static uint64_t EncodeZigzag64(const int64_t& v) {
    if( v < 0 ){
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

/**
 * @brief Zigzag 解码
 * @param v
 * @return
 */
static uint32_t DecodeZigzag32(const int32_t& v) {
    // 将编码值右移一位以恢复整数的绝对值。(去除 ×2 操作)
    // 如果最低位为 1，表示负数，按位取反来得到原始的负数
    return (v >> 1) ^ -(v & 1);
}
static uint64_t DecodeZigzag64(const int64_t& v) {
    return (v >> 1) ^ -(v & 1);
}

/// 有符号整数 先使用 Zigzag 编码 再写入无符号整数
void ByteArray::writeInt32 (int32_t value){
    writeUint32(EncodeZigzag32(value));
}

/// 无符号整数     使用变长编码 数值以7位为一组，每组最高位表示是否还有下一组
void ByteArray::writeUint32 (uint32_t value){
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {                 // 如果数值大于 0x80（128），则使用高位的标记位（0x80）
        tmp[i++] = (value & 0x7F) | 0x80;  // 取低 7 位并设置最高位
        value >>= 7;                       // 将剩余的高位移动到低位
    }
    tmp[i++] = value;            // 最后一个字节不需要设置高位标记
    write(tmp, i);      // 将所有字节写入 ByteArray
}

void ByteArray::writeInt64 (int64_t value){
    writeUint64(EncodeZigzag64(value));
}
void ByteArray::writeUint64 (uint64_t value){
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

/// 单精度浮点数 转换为 uint32_t 再写入
void ByteArray::writeFloat (float value){
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}
/// 双精度浮点数 转换为 uint64_t 再写入
void ByteArray::writeDouble (double value){
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}


/// 不同长度的string类型


/// length:int16 ,  2字节， data
void ByteArray::writeStringF16(const std::string& value){
    writeFuint16(value.size());     // 写入 16 位无符号整数的字符串长度
        write(value.c_str(), value.size());   // 写入字符串的内容
}
/// length:int32 , 4字节 ，  data
void ByteArray::writeStringF32(const std::string& value){
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

/// length:int64   8字节, data
void ByteArray::writeStringF64(const std::string& value){
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

/// length:varint , data
void ByteArray::writeStringVint(const std::string& value){
    writeUint64(value.size());    // 使用 变长编码（Varint）来存储字符串长度
    write(value.c_str(), value.size());
}

void ByteArray::writeStringWithoutLength(const std::string& value){
    write(value.c_str(), value.size());   // 仅写入字符串的内容，不写入长度
}


/// read

int8_t ByteArray::readFint8(){
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t ByteArray::readFuint8(){
    int8_t v;
    read(&v, sizeof(v));  // 直接读取并存储到 变量 v 中
    return v;
}

#define XX(type) \
    type(v);     \
    read(&v, sizeof(v)); \
    if(m_endian == SYLAR_BYTE_ORDER) {  \
        return v; \
    } else {     \
        return byteswap(v);      \
    }


int16_t ByteArray::readFint16(){
    XX(int16_t);
}
uint16_t ByteArray::readFuint16(){
    XX(uint16_t);
}

int32_t ByteArray::readFint32(){
    XX(int32_t);
}
uint32_t ByteArray::readFuint32(){
    XX(uint32_t);
}

int64_t ByteArray::readFint64(){
    XX(int64_t);
}
uint64_t ByteArray::readFuint64(){
    XX(uint64_t);
}

#undef XX

/// 有符号整型   先 读取无符号整型，在进行解码
int32_t ByteArray::readInt32(){
    return DecodeZigzag32(readUint32());
}
/// 无符号整型 使用变长整数（Varint）编码读取 32 位无符号整数
uint32_t ByteArray::readUint32(){
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80){
            result |= ((uint32_t)b) << i;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

int64_t ByteArray::readInt64(){
    return DecodeZigzag64(readUint64());
}
uint64_t ByteArray::readUint64(){
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80){
            result |= ((uint64_t)b) << i;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

// 读取一个 32无符号整数 再转换为 float
float ByteArray::readFloat(){
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}
// 读取一个 64无符号整数 再转换为 double
double ByteArray::readDouble(){
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

/// read string

/// length:int16 , data
std::string ByteArray::readStringF16(){
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
/// length:int32 , data
std::string ByteArray::readStringF32(){
        uint32_t len = readFuint32();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }
/// length:int64 , data
std::string ByteArray::readStringF64(){
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}
/// length:varint , data
std::string ByteArray::readStringVint(){
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}


/// 内部操作

void ByteArray::clear(){
    // 重置位置和大小
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    // 释放链表节点
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}


void ByteArray::write(const void* buf, size_t size){
    if(size == 0) {
        addCapacity(size);  // 扩展容量
    }

    // 当前节点的偏移量（即当前写入的位置）
    size_t npos = m_position % m_baseSize;
    // 当前节点剩余的可用空间大小
    size_t ncap = m_cur->size - npos;
    // 用于标记写入源缓冲区 buf 的位置偏移
    size_t bpos = 0;

    while(size > 0) {
        /// 如果当前节点剩余空间大于或等于要写入的大小   直接写入
        if(ncap >= size) {
            // 写入数据 复制到当前节点 m_cur
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            // 如果当前节点写满 更新到下一个节点
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            // 更新写入位置和缓冲区位置 表示数据已完全写入
            m_position += size;
            bpos += size;
            size = 0;
        } else {      /// 如果当前节点剩余空间小于要写入的大小  先写满当前节点，再移动到下一节点
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            m_position += ncap;      // 更新当前位置
            bpos += ncap;            // 更新缓冲区偏移量
            size -= ncap;            // 减少待写入的字节数
            m_cur = m_cur->next;     // 移动到下一个节点
            ncap = m_cur->size;      // 更新当前节点剩余空间
            npos = 0;                // 重新从节点头部开始写入
        }
    }
    // 更新 m_size 实际大小
    if(m_position > m_size) {
        m_size = m_position;
    }
}

void ByteArray::read(void* buf, size_t size){
    if(size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    while(size > 0) {
        /// 如果当前节点剩余空间大于或等于要读取的大小   直接读取
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {      /// 如果当前节点剩余空间小于要读取的大小  先读取当前节点，再移动到下一节点
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}


void ByteArray::read(void* buf, size_t size, size_t position) const {
    if(size > m_size - position) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}


void ByteArray::setPosition(size_t v){
    if(v > m_capacity){
        throw std::out_of_range("set_position out of range");
    }
    // 更新 m_position 和 m_size
    m_position = v;
    if(m_position > m_size) {
        m_size = m_position;
    }
    // 重新定位 m_cur 指针
    m_cur = m_root;
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    // 检查 v 是否恰好位于当前节点的末尾
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}

bool ByteArray::writeToFile(const std::string& name) const{
    // 打开文件，如果文件不存在则创建，如果存在则清空文件
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        SYLAR_LOG_ERROR(g_logger) << " writeToFile name=" << name
                << "error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    // 获取当前可读数据的大小
    int64_t read_size = getReadSize();
    int64_t pos = m_position;  // 当前读取位置
    Node* cur = m_cur;         // 当前节点指针

    // 遍历每个节点，逐步写入数据
    while (read_size > 0) {
        int diff = pos % m_baseSize;  // 当前节点的偏移
        // 计算要写入的数据长度（确保不超过节点大小）
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;

        // 将当前节点的数据写入文件
        ofs.write(cur->ptr + diff , len);

        // 移动到下一个节点
        cur = cur->next;
        pos += len;        // 更新读取位置
        read_size -= len;  // 更新剩余待写入的数据量
    }
    return true;
}

bool ByteArray::readFromFile(const std::string& name){
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs) {
        SYLAR_LOG_ERROR(g_logger) << " readFromFile name=" << name
                                  << "error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    // 为读取数据分配缓冲区
    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) -> void { delete[] ptr; });

    // 持续读取文件内容直到文件结束
    while (!ifs.eof()) {
        // 读取文件中的数据到缓冲区
        ifs.read(buff.get(), m_baseSize);
        // 将读取到的数据写入到 ByteArray 中
        write(buff.get(), ifs.gcount());
    }
    return true;
}


std::string ByteArray::toString() const{
    std::string str;
    str.resize(getReadSize());
    if(str.empty()){
        return str;
    }
    // 从当前位置开始读取数据 填充到字符串中
    read(&str[0], str.size(), m_position);
    return str;
}

std::string ByteArray::toHexString() const{
    std::string str = toString();
    // 创建一个字符串流来存储转换后的十六进制字符串
    std::stringstream ss;

    // 遍历字符串中的每个字符
    for (size_t i = 0; i < str.size(); ++i) {
        // 每32个字符换行一次
        if (i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        // 以2位十六进制格式输出每个字符的值，填充零   (格式:FF FF FF)
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }
    // 返回转换后的十六进制字符串
    return ss.str();
}

/// 获取用于读取的缓冲区，并将其存储在 iovec 向量中
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const{
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0){
        return 0;
    }
    // 初始化变量
    uint64_t size = len;
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    // 循环直到读取完指定长度的数据
    while(len > 0){
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);   // 将缓冲区信息存入 buffers 向量
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const{
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0){
        return 0;
    }

    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }

    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    while(len > 0){
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len){
    if(len == 0){
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0){
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

/// 扩展 ByteArray 的容量
void ByteArray::addCapacity(size_t size){
    if(size == 0) return;

    size_t old_cap = getCapacity();  // 获取当前容量
    // 如果当前容量已经足够，则无需扩展
    if (old_cap >= size) return;

    // 计算需要增加的容量
    size = size - old_cap;
    // 计算需要新增的节点数
    size_t count = ceil(1.0 * size / m_baseSize);

    Node* tmp = m_root;
    // 找到链表的最后一个节点
    while (tmp->next) {
        tmp = tmp->next;
    }

    Node* first = nullptr;
    // 根据需要的容量，新增相应数量的节点
    for (size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);  // 创建新的节点，并加入链表
        if (first == nullptr) {
            first = tmp->next;  // 记录第一个新增的节点
        }
        tmp = tmp->next;  // 移动到下一个节点
        m_capacity += m_baseSize;  // 更新总容量
    }
    // 如果原来的容量为0，设置当前节点为新增的第一个节点
    if (old_cap == 0) {
        m_cur = first;
    }
}


}
