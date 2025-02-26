/**
  ******************************************************************************
  * @file           : bytearray.h
  * @author         : 18483
  * @brief          : 二进制数组 (序列化 \ 反序列化)
  * @attention      : None
  * @date           : 2025/2/25
  ******************************************************************************
  */


#ifndef SYLAR_BYTEARRAY_H
#define SYLAR_BYTEARRAY_H

#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>

namespace sylar {


/**
 * @brief 二进制数组 提供基础类型的序列化 | 反序列化功能
 */
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    /**
     * @brief ByteArray 的存储节点
     */
    struct Node {

        /**
         * @brief 构造函数  构造指定大小的内存块
         * @param s 内存块字节数
         */
        Node(size_t s);

        /**
         * @brief 无参构造
         */
        Node();

        /**
         * @brief 析构函数 释放内存
         */
        ~Node();

        /// 内存块地址指针
        char* ptr;
        /// 下一个内存块
        Node* next;
        /// 内存块大小
        size_t size;
    };

    /**
     * @brief 使用指定长度的内存块构造 ByteArray
     * @param base_size 内存块大小
     */
    ByteArray(size_t base_size = 4096);

    /**
     * @brief 析构函数
     */
    ~ByteArray();

    ///************************ write ***************************///

    /**
     * @brief 写入固定长度 int8_t类型数据
     * @post m_position += sizeof(value)
     *        如果 m_position > m_size 则 m_size = m_position
     */
    void writeFint8 (int8_t value);
    /**
     * @brief 写入固定长度 uint8_t类型数据
     */
    void writeFuint8 (uint8_t value);
    /**
     * @brief 写入固定长度 int16_t、int32_t、int64_t类型数据 （大端/小端）
     */
    void writeFint16 (int16_t value);
    void writeFuint16 (uint16_t value);

    void writeFint32 (int32_t value);
    void writeFuint32 (uint32_t value);

    void writeFint64 (int64_t value);
    void writeFuint64 (uint64_t value);

    /**
     * @brief 写入有符号Varint32类型数据
     * @post m_position += 实际占用内存 (1 ~ 5)
     *         如果 m_position > m_size 则 m_size = m_position
     */
    void writeInt32 (int32_t value);
    /**
     * @brief 写入 无符号Varint32类型数据
     */
    void writeUint32 (uint32_t value);
    /**
     * @brief 写入有符号Varint64类型数据
     * @post m_position += 实际占用内存 (1 ~ 10)
     *         如果 m_position > m_size 则 m_size = m_position
     */
    void writeInt64 (int64_t value);
    void writeUint64 (uint64_t value);

    /**
     * @brief 写入 float / double 类型数据
     * @post m_position += sizeof(value)
     *        如果 m_position > m_size 则 m_size = m_position
     */
    void writeFloat (float value);
    void writeDouble (double value);

    ///************************ write string ***************************///

    /**
     * @brief 写入 std::string 类型数据 用 uint16_t / uint32_t / uint64_t / Varint64 作为长度类型
     *  @post m_position += 2 / 4 / 8 / Varint64长度  +  value.size()
     *        如果 m_position > m_size 则 m_size = m_position
     */
    /// length:int16 , data
    void writeStringF16(const std::string& value);
    /// length:int32 , data
    void writeStringF32(const std::string& value);
    /// length:int64 , data
    void writeStringF64(const std::string& value);
    /// length:varint64 , data
    void writeStringVint(const std::string& value);
    /**
     * @brief 写入 std::string 类型数据 ， 无长度
     *  @post m_position += value.size()
     *        如果 m_position > m_size 则 m_size = m_position
     */
    void writeStringWithoutLength(const std::string& value);

    ///************************ read ***************************///

    /**
     * @brief 读取 int8_t / int16_t / int32_t / int64_t 类型的数据
     * @pre getReadSize() >= sizeof(intX_t)
     * @post m_position += sizeof(intX_t)
     * @exception getReadSize() < sizeof(intX_t) 抛出 std::out_of_range
     */
    int8_t readFint8();
    uint8_t readFuint8();

    int16_t readFint16();
    uint16_t readFuint16();

    int32_t readFint32();
    uint32_t readFuint32();

    int64_t readFint64();
    uint64_t readFuint64();

    /**
     * @brief 读取 有/无 符号 的 Varint32 / Varint64 类型的数据
     * @pre getReadSize() >= 类型实际占用内存
     * @post m_position += 类型实际占用内存
     * @exception 如果getReadSize() < 类型实际占用内存 抛出 std::out_of_range
     */
    int32_t readInt32();
    uint32_t readUint32();

    int64_t readInt64();
    uint64_t readUint64();

    /**
     * @brief 读取 float / double 类型的数据
     * @pre getReadSize() >= sizeof(float / double)
     * @post m_position += sizeof(float / double)
     * @exception getReadSize() < sizeof(float / double) 抛出 std::out_of_range
     */
    float readFloat();
    double readDouble();

    ///************************ read string ***************************///

    /**
     * @brief 读取 std::string 类型的数据 用 uint16_t / uint32_t / uint64_t / Varint64 作为长度类型
     * @pre getReadSize() >= sizeof(uintXX_t) + size
     * @post m_position += sizeof(uintXX_t) + size
     * @exception getReadSize() < sizeof(uintXX_t) + size 抛出 std::out_of_range
     */
    /// length:uint16 , data
    std::string readStringF16();
    /// length:uint32 , data
    std::string readStringF32();
    /// length:uint64 , data
    std::string readStringF64();
    /// length:varint , data
    std::string readStringVint();


    ///************************ 内部操作 ***************************///

    /**
     * @brief 清空 ByteArray
     * @post m_position= 0 , m_size = 0
     */
    void clear();

    /**
     * @brief 写入 size 长度的数据
     * @param buf 内存缓存指针
     * @param size 数据大小
     */
    void write(const void* buf, size_t size);
    /**
     * @brief 读取 size 长度的数据
     * @param buf 内存缓存指针
     * @param size 数据大小
     */
    void read(void* buf, size_t size);
    /**
     * @brief 读取 size 长度的数据
     * @param buf 内存缓存指针
     * @param size 数据大小
     * @param position 读取开始的位置
     */
    void read(void* buf, size_t size, size_t position) const;

    /**
     * @brief 返回 ByteArray 当前位置
     */
    size_t getPosition() const { return m_position;}

    /**
     * @brief 设置 ByteArray 当前位置
     * @post 如果如果m_position > m_size 则 m_size = m_position
     * @exception 如果m_position > m_capacity 则抛出 std::out_of_range
     */
    void setPosition(size_t v);

    /**
     * @brief 把 ByteArray 数据写入到文件
     * @param name 文件名
     */
    bool writeToFile(const std::string& name) const;
    /**
     * @brief 从文件读取 ByteArray 数据
     * @param name 文件名
     */
    bool readFromFile(const std::string& name);

    /**
     * @brief 返回内存块大小
     */
    size_t getBaseSize() const {return m_baseSize;}
    /**
     * @brief 返回可读数据大小
     */
    size_t getReadSize() const {return m_size - m_position;}

    /**
     * @brief 是否是小端
     */
    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    /**
     * @brief 将 ByteArray 里面的数据[m_position, m_size) 转换成 std::string
     */
    std::string toString() const;
    /// 转换为 16进制 (格式:FF FF FF)
    std::string toHexString() const;

    /**
     * @brief 获取可读取的缓存 保存iovec数组
     * @param buffers 保存可读取数据的iovec数组
     * @param len 读取数据的长度 如果 len > getReadSize() 则 len = getReadSize()
     * @return 返回实际数据的长度
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    /**
     * @param position 读取数据的位置
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    /**
     * @brief 获取可写入的缓存 保存iovec数组
     * @param buffers 保存可写入数据的iovec数组
     * @param len 写入数据的长度
     * @return 返回实际数据的长度
     * @post 如果 (m_position + len) > m_capacity 则 m_capacity 扩容 N 个节点以容纳 len 长度
     */
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    /**
     * @brief 返回数据的长度
     */
    size_t getSize() const {return m_size;}

private:

    /**
     * @brief 扩容 ByteArray 使其可以容纳 size 个数据
     * @param size
     */
    void addCapacity(size_t size);

    /**
     * @brief 获取当前的可写入容量
     */
    size_t getCapacity() const {return m_capacity - m_position; }

private:
    /// 内存块大小
    size_t m_baseSize;
    /// 当前操作位置
    size_t m_position;
    /// 当前总容量
    size_t m_capacity;
    /// 当前数据大小
    size_t m_size;
    /// 字节序 默认大端
    int8_t m_endian;
    /// 第一个内存块指针
    Node* m_root;
    /// 当前操作的内存块指针
    Node* m_cur;

};


}


#endif //SYLAR_BYTEARRAY_H
