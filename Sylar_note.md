**相关C++知识点整理**

项目地址：

[GitHub - Lwaiz/sylar_leo](https://github.com/Lwaiz/sylar_leo)


# 目录

日志系统 [Logger](#日志系统)

配置系统 [Config](#配置系统)

线程模块 [Thread](#线程模块)

协程模块 [Fiber](#协程模块)

协程调度模块 [Scheduler](#协程调度模块)

IO协程调度模块 [IOManager](#IO协程调度模块)

定时器模块 [Timer](#定时器模块)

Hook模块 [Hook](#Hook模块)

地址模块 [Address](#Address模块)

网络模块  [Socket](#Socket模块)

序列化模块 [ByteArray](#序列化模块)

# CMake




# 日志系统

## 宏定义

**宏定义**是 C 和 C++ 语言中的一种**预处理指令**，用于**在编译前对代码进行文本替换和处理。**宏定义由 `#define` 关键字引入，是一种灵活且强大的工具。

- 在宏定义中，反斜杠 `\` 的作用是 **用于将宏定义扩展到下一行**，即让宏在逻辑上仍然是一个连续的定义，而不会被换行打断。

    - 不能在最后一行使用`\`，预处理器会认为宏没有结束

    - `\` 后不能有空格或其他字符

**基本形式：**`#define 宏名 替换文本`

**带参数的宏：**`#define 宏名(参数列表) 替换文本`

**取消宏定义：**`#undef 宏名`

### 特殊的宏

预处理器提供了一些特殊宏，可用于调试或代码生成：

- `__FILE__`：当前文件名。

- `__LINE__`：当前行号。

- `__DATE__`：编译日期。

- `__TIME__`：编译时间。

- `__cplusplus`：C++ 标准版本。

### 日志流写入宏

使用 `SYLAR_LOG_LEVEL` 宏 和对应级别宏（如 `SYLAR_LOG_DEBUG`）支持通过流式操作输出日志信息

- 检查 `logger` 的当前日志级别是否小于等于传入的日志级别 `level`

- 检查机制，只记录满足条件的日志，可以控制日志的详细程度。

    - DEBUG 级别的日志可能会记录所有内容。

    - WARN 或 ERROR 级别的日志只记录重要信息。

- 使用`LogEvent`创建一个日志事件并使用`LogEventWrap`包装起来

- 调用 `sylar::LogEventWrap` 对象的 `getSS()` 方法

    - 返回一个 `std::stringstream` 对象，用于接收日志的内容（即将日志写入到流中）

```C++
/**
 * @brief 使用流方式将日志级别 level 的日志写到 logger
 */
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level)    \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent( \
                                logger, level, __FILE__, __LINE__, 0, \
                                sylar::GetThreadId(),          \
                                sylar::GetFiberId(),           \
                                time(0),                       \
                                "main_thread"))).getSS()

/**
 * @brief 使用流方式将 debug、info、warn、error、fatal 等不同级别的日志写到 logger
 * 
 * 说明：
 * - 每个宏都会调用 `SYLAR_LOG_LEVEL`，只需传入相应的日志级别即可。
 */
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
```

### switch-case 语句

使用 `switch-case` 语句根据 `level` 的值返回相应的字符串。

宏定义 `XX(name)` 用于生成 `case` 分支的代码：

```C++
#define XX(name) \
    case LogLevel::name: \
        return #name; \      //#name 是将对应的参数名称变成了字符串
        break;
```

展开后，效果类似于：

```C++
case LogLevel::DEBUG:
    return "DEBUG";
    break;
case LogLevel::INFO:
    return "INFO";
    break;
// 以此类推...
```

宏定义的作用是减少重复代码的编写，提高可维护性。当需要新增日志级别时，只需在 `XX(...)` 中添加相应条目即可。

`#undef XX` 是为了避免宏污染，即在宏使用完后取消其定义。

## enum枚举类型

**enum枚举类型** 提供了一种方法来命名整数常量，使得代码更加易读和可维护

枚举类型 `Level` 用于定义日志系统中的不同日志级别。每个级别都有一个对应的命名常量，它们的值是从 `0` 到 `5`

```C++
enum Level {
    UNKNOW = 0,  // 未知级别，通常用于初始化或错误状态
    DEBUG = 1,   // 调试级别，用于开发和调试程序时输出详细信息
    INFO = 2,    // 信息级别，用于记录正常运行时的重要信息
    WARN = 3,    // 警告级别，用于提示潜在问题
    ERROR = 4,   // 错误级别，表示程序运行中遇到了问题
    FATAL = 5    // 致命错误级别，系统级别的严重错误
};
```

C++11 引入了 **`enum class`**，也称为 **强类型枚举**，它提供了类型安全的枚举，避免了与其他类型的隐式转换。

**`enum` 和 `enum class` 的对比**

|特性|`enum`|`enum class`|
|-|-|-|
|是否支持类型安全|不支持（隐式转换）|支持（没有隐式转换）|
|使用时是否需要指定作用域|否|需要指定作用域|
|可以与整数互相转换|是|否|

## va_list 和 vasprintf

`va_list` 是 C/C++ 中用于**处理可变参数函数的类型**。可变参数函数允许我们传递不定数量的参数，像 `printf` 这样的标准函数就是一个典型的例子。

定义和操作可变参数需要以下宏：

1. **`va_start`**：初始化 `va_list`，用于访问可变参数。

2. **`va_arg`**：获取下一个参数。

3. **`va_end`**：结束对 `va_list` 的访问。

4. **`va_copy`**：复制一个 `va_list`。

`vasprintf` 是一个 C 标准库扩展函数（POSIX 标准），它基于格式化字符串生成动态分配的字符串。

`int vasprintf(char **strp, const char *fmt, va_list ap);`

- **`strp`**：指向生成的字符串指针的指针。

- **`fmt`**：格式化字符串。

- **`ap`**：包含可变参数的 `va_list`

- 返回生成的字符串长度（不包括末尾的 `\0`）。

    - 如果分配失败，则返回 `-1`。

**`va_list` 提供了访问可变参数的能力**

**`vasprintf` 提供了动态分配内存并格式化字符串的能力**

```C++
/// LogEvent 的格式化日志输出功能
/// 接受可变参数的format函数
void LogEvent::format(const char* fmt, ...){
    va_list al;               // 定义可变参数列表
    va_start(al, fmt);        // 初始化可变参数列表
    format(fmt, al);          // 调用重载方法处理格式化
    va_end(al);               // 结束可变参数列表
}

///实际处理字符串格式化的函数
void LogEvent::format(const char* fmt, va_list al){
    char* buf = nullptr;  //定义字符指针 用于存储格式化后的字符串
    // 使用 vasprintf 动态分配缓冲区并格式化字符串
    int len = vasprintf(&buf, fmt, al);

    //检查格式化是否成功
    if(len != -1) {
        //将格式化后的字符串追加到字符串流中
        m_ss << std::string(buf, len);
        //释放动态分配缓冲区
        free(buf);
    }
}
```

## Shared_Ptr指针

### `enable_shared_from_this`

`enable_shared_from_this` 是 C++11 提供的一个标准库工具，主要用于在一个类的成员函数内部安全地生成当前对象的 `std::shared_ptr` 实例

- 通常情况下，当对象已经由 `std::shared_ptr` 管理时，直接使用 `std::make_shared` 会导致 `std::shared_ptr` 的引用计数被重复管理，可能引发未定义行为。`std::enable_shared_from_this` 通过维护一个弱引用解决了这个问题。

**继承**：类需要继承 `std::enable_shared_from_this<T>`，其中 `T` 是当前类的名字。

**只能使用 `shared_from_this`**：

- 当前对象必须由 `std::shared_ptr` 管理。

- 如果不是由 `std::shared_ptr` 管理，调用 `shared_from_this` 会引发未定义行为。

```C++
class Logger : public std::enable_shared_from_this<Logger>{
public:
    typedef std::shared_ptr<Logger> ptr;
    
    ......  
}；
```

## 获取线程ID

```C++
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

pid_t GetThreadId(){
    return syscall(SYS_gettid);
}
```

## 单例模式

**单例模式**是指在整个系统生命周期内，保证一个类只能产生一个实例，确保该类的唯一性。

1. **节省资源**。一个类只有一个实例，不存在多份实例，节省资源。

2. **方便控制**。在一些操作公共资源的场景时，避免了多个对象引起的复杂操作。

### 单例模式的特点

1. **唯一性**：类的实例是唯一的，不能创建多个实例。

2. **全局访问**：提供一个全局的访问方法来获取该实例。

3. **延迟实例化**：通常采用延迟实例化技术，只有在需要使用实例时才创建。

### 单例模式实现方式

- 将其构造和析构成为私有的, 禁止外部构造和析构

- 将其拷贝构造和赋值构造成为私有函数, 禁止外部拷贝和赋值

单例模式可以分为 懒汉式 和 饿汉式 ，两者之间的区别在于创建实例的时间不同。

- **懒汉式（Lazy Initialization）**

    系统运行中，实例并不存在，只有当第一次需要使用该实例时，才会去创建并使用实例。这种方式要考虑线程安全。

    - 延迟加载，节省资源

    - 不够线程安全，多线程需要加锁

    - 通过成员函数创建，调用才创建

- **饿汉式（Eager Initialization）**

    系统一运行，就初始化创建实例，当需要时，直接调用即可。这种方式本身就线程安全，没有多线程的线程安全问题。

    - 线程安全

    - 浪费内存，即使不用也会创建一个实例 

    - 静态实例作为成员变量，程序加载时创建

**模板实现**

- `Singleton`  返回类 `T` 的单例裸指针形式

- 使用了一个 `static` 局部变量 `v` 来确保只创建一个 `T` 类型的实例

- 将`LoggerManager`单例模式类定义为`LoggerMgr`

```C++
/**
 * @brief    单例模式 裸指针封装类
 * @tparam T    类型
 * @tparam X    为了创造多个实例对应的 Tag
 * @tparam N    同一个 Tag 创造了多个实例索引
 */
template <class T, class X = void, int N = 0>
class Singleton {
public:
    /**
     * @brief 返回单例裸指针
     *      延迟初始化 只有在第一次调用GetInstance 时创建实例
     */
    static T* GetInstance(){
        static T v;    ///static 确保全局只有一个实例
        return &v;
    }
};
/**
 * @brief    单例模式 智能指针封装类
 */
    template <class T, class X = void, int N = 0>
    class SingletonPtr {
    public:
        /**
         * @brief 返回单例智能指针
         * @return
         */
        static std::shared_ptr<T> GetInstance(){
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };

/// 日志管理类应用单例模式
typedef sylar::Singleton<LoggerManager> LoggerMgr;
/// 单例模式使用 LoggerMgr
// 通过名称 xx 获取一个日志器实例 并记录日志
auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
SYLAR_LOG_INFO(l) << "xxx";
```



## test模块

```C++
#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"

int main(int argc,char** argv){

    //创建日志器实例
    sylar::Logger::ptr logger(new sylar::Logger);

    //创建标准输出日志目标 并 添加到日志器中
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    //创建文件输出日志目标
    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    logger->addAppender(file_appender);

    //配置文件流日志格式器
    // 配置日志格式 并设置能够写入文件流的最低日志级别
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d{%Y-%m-%d %H:%M:%S} [%p] (%f:%l) %m %n"));
    file_appender->setFormatter(fmt);
    //file_appender->setLevel(sylar::LogLevel::ERROR);
    file_appender->setLevel(sylar::LogLevel::INFO);

    //创建日志事件
    sylar::LogEvent::ptr event(new sylar::LogEvent(logger,
                                                   sylar::LogLevel::DEBUG,
                                                   __FILE__,
                                                   __LINE__,
                                                   0,
                                                   sylar::GetThreadId(),
                                                   sylar::GetFiberId(),
                                                   time(0),
                                                   "main_thread")
                                                   );
    //使用 logger->log 方法记录日志事件
    event->getSS() << "hello sylar log";
    logger->log(sylar::LogLevel::DEBUG, event);
    std::cout << "hello sylar log" << std::endl;

    //使用宏记录 流日志
    SYLAR_LOG_INFO(logger) << " test macro" ;
    SYLAR_LOG_ERROR(logger) << " test macro error";
    //使用宏记录 格式化日志
    SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
    //通过名称 xx 获取一个日志器实例 并记录日志
    auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";

    return 0;
}

```

**Logger模块测试结果**

```Shell
##stdout
2025-01-14 21:59:17	15144	main_thread	0	[DEBUG]	[root]	</home/leo/CppProgram/sylar/tests/test.cpp:28>	hello sylar log
hello sylar log
2025-01-14 21:59:17	15144	main_thread	0	[INFO]	[root]	</home/leo/CppProgram/sylar/tests/test.cpp:41>	 test macro
2025-01-14 21:59:17	15144	main_thread	0	[ERROR]	[root]	</home/leo/CppProgram/sylar/tests/test.cpp:42>	 test macro error
2025-01-14 21:59:17	15144	main_thread	0	[ERROR]	[root]	</home/leo/CppProgram/sylar/tests/test.cpp:44>	test macro fmt error aa
2025-01-14 21:59:17	15144	main_thread	0	[INFO]	[root]	</home/leo/CppProgram/sylar/tests/test.cpp:47>	xxx

##fileout
2025-01-14 21:59:17 [INFO] (/home/leo/CppProgram/sylar/tests/test.cpp:41)  test macro 
2025-01-14 21:59:17 [ERROR] (/home/leo/CppProgram/sylar/tests/test.cpp:42)  test macro error 
2025-01-14 21:59:17 [ERROR] (/home/leo/CppProgram/sylar/tests/test.cpp:44) test macro fmt error aa 

```

# 配置系统

## boost库

`Boost` 是一个流行的 C++ 开源库集合，提供了许多高效的库和工具来简化 C++ 开发。

Boost 包含一组经过广泛测试的库，涵盖领域包括：

- 文件系统（`Boost.Filesystem`）

- 多线程（`Boost.Thread`）

- 正则表达式（`Boost.Regex`）   <纳入 C++ 标准库>

- 智能指针（`Boost.SmartPtr`）  <纳入 C++ 标准库>

- 网络库（`Boost.Asio`）

- 序列化（`Boost.Serialization`）

```C++
leo@MateBook_Leo:~/CppProgram/sylar$ sudo apt-get install libboost-all-dev  //安装boost库
......
leo@MateBook_Leo:~/CppProgram/sylar$ cat /usr/include/boost/version.hpp | grep "BOOST_LIB_VERSION"  
  //查看boost版本
//  BOOST_LIB_VERSION must be defined to be the same as BOOST_VERSION
#define BOOST_LIB_VERSION "1_74"
```

### **lexical_cast**

**`boost::lexical_cast`** 是 Boost 库中提供的一种**轻量级类型转换工具**，它可以在字符串与常见类型（如整数、浮点数、自定义类）之间进行高效的转换。它的设计目标是简洁、易用和安全。

`boost::lexical_cast<目标类型>(待转换值)`

```C++
std::string toString() override{
        try{
            return boost::lexical_cast<std::string>(m_val);
        } catch(std::exception& e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

bool fromString(const std::string& val) override {
        try {
            m_val = boost::lexical_cast<T>(val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception"
                 << e.what() << " convert: string to " << typeid(m_val).name();
        }
        return false;
    }
```

### 模板类偏特化

**模板类的偏特化（template specialization）**是指为某些特定类型的模板参数提供专门的实现，区别于通用模板实现。

在C++中，模板有两种特化方式：

1. **全特化（Full Specialization）**：为模板参数完全指定类型的特化。例如，模板 `T` 为 `int` 类型时的实现。

2. **偏特化（Partial Specialization）**：为模板参数指定部分类型的特化。允许将某些类型的模板参数进行特化，而不需要完全指定。

```C++
/**
 * @brief 类型转换模板类 (F 源类型, T 目标类型)
 */
template<class F, class T>
class LexicalCast{
public:
    /**
     * @brief 类型转换
     * @param v 源类型值
     * @return 返回 v 转换后的目标类型
     * @exception 当类型不可转换时抛出异常
     */
    T operator()(const F& v){
        return boost::lexical_cast<T>(v);
    }
};

/// 1.vector<T>
/**
 * @brief 类型转换模板类偏特化(YAML String 转换成 std::vector<T> )
 */
template<class T>
class LexicalCast<std::string, std::vector<T>>{
public:
    /// 重载 () 运算符 仿函数
    std::vector<T> operator()(const std::string& v){
        /// 利用 YAML::Load 将字符串解析为 YAML::Node
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        /// 遍历 YAML::Node 的每一个元素，递归调用转换为目标类型 T 并存入 vec
        for(size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类偏特化( std::vector<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::vector<T>, std::string>{
public:
    std::string operator()(const std::vector<T>& v){
        /// 创建一个 YAML::Node 类型为 Sequence
        YAML::Node node(YAML::NodeType::Sequence);
        /// 遍历 std::vector<T> 的每一个元素，
        /// 递归调用转换为字符串，并加入到 YAML::Node 中
        for(auto & i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        /// 将 YAML::Node 序列化为字符串返回
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
```

## yaml-cpp库

`yaml-cpp` 是一个轻量级的 YAML 解析和生成库，支持 YAML 1.2 标准。

提供了简单直观的 API，可用于读取和写入 YAML 文件。

**安装步骤**

- 下载yaml-cpp库

    -  `git clone https://github.com/jbeder/yaml-cpp.git`

- 编译安装

    - `mkdir build` #新建build 文件夹

    - `cd build`

    - `cmake -DBUILD_SHARED_LIBS=ON ..` #ON 设置生成共享库

    - `sudo make install`

- 验证安装

    - `pkg-config --modversion yaml-cpp`

```PowerShell
leo@MateBook_Leo:~/CppProgram/sylar$ git clone https://github.com/jbeder/yaml-cpp.git
Cloning into 'yaml-cpp'...
remote: Enumerating objects: 9003, done.
remote: Counting objects: 100% (172/172), done.
remote: Compressing objects: 100% (101/101), done.
remote: Total 9003 (delta 113), reused 71 (delta 71), pack-reused 8831 (from 4)
Receiving objects: 100% (9003/9003), 4.53 MiB | 79.00 KiB/s, done.
Resolving deltas: 100% (5824/5824), done.

leo@MateBook_Leo:~/CppProgram/sylar$ mv yaml-cpp ~/CppProgram/
leo@MateBook_Leo:~/CppProgram/sylar$ cd yaml-cpp
-bash: cd: yaml-cpp: No such file or directory
leo@MateBook_Leo:~/CppProgram/sylar$ cd ~/CppProgram/yaml-cpp/
leo@MateBook_Leo:~/CppProgram/yaml-cpp$ mkdir build
leo@MateBook_Leo:~/CppProgram/yaml-cpp$ cd build/
leo@MateBook_Leo:~/CppProgram/yaml-cpp/build$ cmake -DBUILD_SHARED_LIBS=ON ..
-- Configuring done
-- Generating done
-- Build files have been written to: /home/leo/CppProgram/yaml-cpp/build

leo@MateBook_Leo:~/CppProgram/yaml-cpp/build$ sudo make install
[sudo] password for leo:
Consolidate compiler generated dependencies of target yaml-cpp
[100%] Built target yaml-cpp-read
Install the project...
-- Install configuration: ""
-- Installing: /usr/local/lib/libyaml-cpp.so.0.8.0
-- Installing: /usr/local/lib/pkgconfig/yaml-cpp.pc
leo@MateBook_Leo:~/CppProgram/yaml-cpp/build$ pkg-config --modversion yaml-cpp
0.8.0
```

### YAML::Node

在YAML中，`node` 通常是指解析后的 YAML 数据结构中的一个元素，`YAML::Node` 是 `yaml-cpp` 库中的核心类之一，用于表示 YAML 数据的各个部分。它充当了一个容器，可以持有不同类型的数据，包括标量、序列、映射等。

#### Node类型

1. **Scalar**：

- 一个标量表示 YAML 中的基本值，比如字符串、整数、布尔值等。

- 例如，`"name: John"` 中的 `John` 是一个标量。

- `IsScalar()`判断节点类型

1. **Sequence**：

- 一个序列表示一个有序的集合，对应于 YAML 中的数组或列表。

- 例如，`fruits: [apple, banana, orange]` 中的 `[apple, banana, orange]` 是一个序列。

- `IsSequence()`判断节点类型

1. **Map**：

- 一个映射表示一个无序的键值对集合，对应于 YAML 中的字典或对象。

- 例如，`person: {name: John, age: 30}` 中的 `person` 是一个映射。

- `IsMap()`判断节点类型

```C++
void print_yaml(const YAML::Node& node, int level){
    if(node.IsScalar()){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()){
        for(auto it = node.begin();
                it != node.end(); ++it){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()){
        for(size_t i = 0; i < node.size(); ++i){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}
```

## find_first_not_of

`find_first_not_of` 和 `find_last_not_of` 是 C++ `std::string` 类中用于查找字符或子字符串的成员函数。它们的作用是寻找第一个或最后一个不符合指定字符集的字符

```C++
if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
                    != std::string::npos){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }
```

## std::transform

`std::transform` 是 C++ 标准库中的一个算法，用于对一个序列中的每个元素应用给定的操作，并将结果存储在另一个序列中。它通常用于修改容器或生成一个新的容器

```C++
/// 对单一容器进行操作
// unary_op：单一输入的操作（例如：对每个元素执行某种变换）
std::transform(InputIterator first1, InputIterator last1,
               OutputIterator d_first, UnaryOperation unary_op);
/// 对两个容器进行操作
// binary_op：两个输入序列的元素进行操作的二元操作
std::transform(InputIterator first1, InputIterator last1,
               InputIterator first2, OutputIterator d_first, BinaryOperation binary_op);
```

```C++
std::transform(m_name.begin(), m_name.end(),m_name.begin(), ::tolower);
```

`::tolower` 和 `::toupper` 是 C++ 标准库中的两个函数，分别用于将字符转换为小写和大写字母。它们都是在 `<cctype>` 头文件中定义的。返回值是`int`类型的ASCII编码值

## 事件模式

**"事件模式"（Event-driven Model）**通常指的是一种编程模式，主要用于处理异步事件。在这种模式下，程序的流动不是由函数调用的顺序决定的，而是由外部事件（如用户输入、定时器到期、文件IO等）触发相应的事件处理程序（即回调函数）。

### 回调函数管理

**定义回调函数类型**

```C++
/// 配置事件回调函数（callback -- cb）
typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;
```

1. **回调函数类型**：在 `ConfigVar` 中，定义了一个 `on_change_cb` 类型的回调函数，该函数接收两个参数：旧值和新值。

    ```C++
    typedef std::function<void(const T& old_value, const T& new_value)> on_change_cb;
    ```

1. **添加回调函数**：可以通过 `addListener` 方法添加回调函数，它接受一个唯一的 ID 和回调函数作为参数。

    ```C++
    void addListener(uint64_t key, on_change_cb cb) {
        m_cbs[key] = cb;
    }
    ```

1. **删除回调函数**：通过 `delListener` 方法可以移除指定 ID 的回调函数。

    ```C++
    void delListener(uint64_t key) {
        m_cbs.erase(key);
    }
    ```

1. **调用回调函数**：在 `setValue` 方法中，当配置变量的值发生变化时，会遍历所有注册的回调函数，并调用这些函数来通知它们新旧值的变化。

    ```C++
    void setValue(const T& v) {
        if (v == m_val) {
            return;
        }
        ///值发生变化时 执行回调函数
        for (auto& i : m_cbs) {
            i.second(m_val, v);  // 调用回调函数
        }
        m_val = v;
    }
    ```

### 总结

1. **添加回调**：可以通过 `addListener` 方法注册回调函数。

2. **删除回调**：可以通过 `delListener` 方法移除回调函数。

3. **触发回调**：在 `setValue` 方法中，当配置变量值更改时，会调用所有注册的回调函数。

```C++
g_person->addListener(1000, [](const Person& old_value, const Person& new_value) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << old_value.toString()
                                         << "new_value=" << new_value.toString();
    });
```

这种模式的优势在于可以动态管理配置变量的变化通知，方便系统对配置的响应和监控。

### std::function

**`std::function`**：是 C++ 标准库中的一个模板类，用于存储、传递和调用任何可以调用的目标（如普通函数、成员函数、lambda 表达式、函数对象等）。它提供了一种统一的方式来表示可调用对象

```C++
std::function<返回类型(参数类型)> 函数对象;
std::function<void(int)> func = [](int x) {
        std::cout << "The value is: " << x << std::endl;
};
```

## dynamic_pointer_cast

将一个 `std::shared_ptr`（或 `std::weak_ptr`）指向的对象转换为基类指针指向的对象的派生类指针。它是 `std::shared_ptr` 和 `std::weak_ptr` 的成员函数之一，用于进行类型安全的转换。

```C++
std::dynamic_pointer_cast<目标类型>(被转换智能指针);

auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
```

- 当你有一个指向基类的 `std::shared_ptr`，并希望将其转换为指向派生类的指针时，使用 `std::dynamic_pointer_cast`。

- 它可以避免不安全的类型转换，保证了转换的安全性（如果转换不成功，返回 `nullptr`）。

- std::dynamic_pointer_cast 只有在类有虚函数（即多态）时才能工作，否则转换会失败。



# 线程模块

**进程——资源分配的最小单位，线程——程序执行的最小单位**

## CPU时间片

**时间片即CPU分配给各个程序的时间，每个线程被分配一个时间段，称作它的时间片**。

- 即该进程允许运行的时间，使各个程序从表面上看是同时进行的。

- 如果在时间片结束时进程还在运行，则CPU将被剥夺并分配给另一个进程。

- 如果进程在时间片结束前阻塞或结束，则CPU当即进行切换。

不会造成CPU资源浪费。

在宏观上：我们可以同时打开多个应用程序，每个程序并行不悖，同时运行。

但在微观上：由于只有一个CPU，一次只能处理程序要求的一部分，如何处理公平，一种方法就是**引入时间片，每个程序轮流执行**。

**线程(任务)会因为CPU时间片轮转机制，而不断的切换，且无序！**

## CPU上下文切换

在每个任务运行前，CPU 都需要知道任务从哪里加载、又从哪里开始运行。

也就是说，需要系统事先帮它设置好 CPU 寄存器和程序计数器（Program Counter，PC）。

CPU 寄存器，是 CPU 内置的容量小、但速度极快的内存。

而程序计数器，则是用来存储 CPU 正在执行的指令位置、或者即将执行的下一条指令位置。

它们都是 CPU 在运行任何任务前，必须的依赖环境，因此也被叫做 CPU 上下文。

**上下文就是保存了对应线程状态的【镜像】，切换上下文需要消耗CPU资源**

## 线程

**线程是cpu执行的最小单位，包括线程ID，程序计数器，寄存器集和栈。**

和其他同属于一个进程的其他线程共享操作系统资源：代码区，数据区打开文件和信号。

```C++
//创建线程开始运行相关线程函数，运行结束则线程退出
pthread_create()
//因为exit()是用来结束进程的，所以则需要使用特定结束线程的函数	
pthread_eixt()	
//挂起当前线程，用于阻塞式地等待线程结束，如果线程已结束则立即返回，0=成功
pthread_join()	
//发送终止信号给thread线程，成功返回0，但是成功并不意味着thread会终止
pthread_cancel()	
//在不包含取消点，但是又需要取消点的地方创建一个取消点，以便在一个没有包含取消点的执行代码线程中响应取消请求.
pthread_testcancel()
//设置本线程对Cancel信号的反应	
pthread_setcancelstate()
//设置取消状态 继续运行至下一个取消点再退出或者是立即执行取消动作	
pthread_setcanceltype()	
//设置取消状态
pthread_setcancel()	
```

### Thread类

#### static thread_local

`static thread_local`是`C++`中的一个关键字组合，用于定义静态线程本地存储变量。具体来说，当一个变量被声明为`static thread_local`时，它会在每个线程中拥有自己独立的静态实例，并且对其他线程不可见。这使得变量可以跨越多个函数调用和代码块，在整个程序运行期间保持其状态和值不变。

```C++
// 指向当前线程 
static thread_local Thread *t_thread = nullptr;
// 指向线程名称
static thread_local std::string t_thread_name = "UNKNOW";
```

#### pthread_create() 创建线程

`int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);`，是`POSIX`线程库中的一个函数，用于创建一个新的线程。

- `thread`：指向`pthread_t`类型的指针，用于存储新创建线程的`ID`。

- `attr`：指向`pthread_attr_t`类型的指针，用于设置新线程的属性，一般传入`NULL`表示使用默认属性。

- `start_routine`：一个函数指针，指向新线程将要执行的函数，该函数的原型为`void* (*)(void*)`，接受一个`void*`类型的参数，返回一个`void*`类型的指针。

- `arg`：传递给`start_routine`函数的参数。

调用`pthread_create`函数后，将会创建一个新线程，并开始执行通过`start_routine`传递给它的函数。新线程的`ID`将存储在`thread`指向的变量中。请注意，新线程将在与调用`pthread_create`函数的线程并发执行的情况下运行

#### pthread_detach() 线程分离

`pthread_detach`函数用于释放与线程关联的资源，并确保线程可以安全地终止。

#### join() 等待线程终止

`int pthread_join(pthread_t thread, void **retval);`函数是`POSIX`线程`API`中的一种同步原语，它允许调用线程等待另一个线程的终止。

- 当一个线程调用`pthread_join`时，它会**阻塞**直到指定的线程终止。

- 一旦目标线程终止，调用线程恢复执行，并可以选择通过`retval`参数获取线程的返回值。

- 如果调用线程不关心返回值，也可以将`retval`参数设置为`NULL`。

#### run() 线程执行函数

线程执行函数，通过信号量，能够确保构造函数在创建线程之后会一直阻塞，直到`run`方法运行并通知信号量，构造函数才会返回。

```C++
    std::function<void()> cb;
    cb.swap(thread -> m_cb);//交换线程执行函数
    // 在出构造函数之前，确保线程先跑起来，保证能够初始化id
    thread -> m_semaphore.notify(); 

    cb(); // 执行线程函数
```

## 锁模块

**锁模块封装了信号量、互斥量、读写锁、自旋锁、原子锁**

    - **`Semaphore`：技术信号量，基于`sem_t`实现。**

    - **`Mutex`：互斥锁，基于`pthread_mutex_t`实现。**

    - **`RWMutex`：读写锁，基于`pthread_rwlock_t`实现。**

    - **`Spinlock`：自旋锁，基于`pthread_spinlock_t`实现。**

    - **`CASLock`：原子锁，基于`std::atomic_flag`实现**

### Semaphore 信号量  

信号量是用来解决线程同步和互斥的通用工具， 和互斥量类似。

信号量也可以用作自于资源互斥访问， 但信号量没有所有者的概念，在应用上比互斥量更广泛，信号量比较简单， 不能解决优先级反转问题。

但信号量是一种轻量级的对象，比互斥量小巧，灵活，因此在很多对互斥要求不严格的的系统中，经常使用信号量来管理互斥资源。

如果定义的信号量表示一种资源，则它是用来同步的，如果信号量定义成一把锁，则它是用来保护的。

#### sem_init() 初始化信号量

- `sem`：指向`sem_t`类型的指针，用于存储信号量对象。

- `pshared`：如果非零，表示这个信号量可以被其他进程访问；如果为零，表示这个信号量只对当前进程内的线程可见。

- `value`：信号量的初始值。在信号量中，这个值通常用来表示资源的数量或者可用的某种事物的数量

#### sem_destory() 销毁信号量

`int sem_destroy(sem_t *sem)`

- 只有在确保没有任何线程或进程正在使用该信号量时，才应该调用`sem_destroy()`函数。

- 如果在调用`sem_destroy()`函数之前，没有使用`sem_post()`函数将信号量的值增加到其初始值，则可能会导致在销毁信号量时出现死锁情况。

#### wait() 等待信号量

`int sem_wait(sem_t *sem)`

- 如果在调用此函数时信号量的值大于零，则该值将递减并立即返回。

- 如果信号量的值为零，则当前线程将被阻塞，直到信号量的值大于零或者被信号中断。

#### notify() 通知信号量

`int sem_post(sem_t *sem);`用于向指定的命名或未命名信号量发送信号，使其计数器加`1`。如果有进程或线程正在等待该信号量，那么其中一个将被唤醒以继续执行。

### Mutex 互斥锁

**互斥是进程(线程)之间的间接制约关系。**

当一个进程(线程)进入临界区使用临界资源时，另一个进程(线程)必须等待。

只有当使用临界资源的进程(线程)退出临界区后，这个进程(线程)才会解除阻塞状态。

**临界资源**

- 对于某些资源来说，其在同一时间只能被一个进程所占用。

- 这些一次只能被一个进程所占用的资源就是所谓的临界资源。

**临界区**

- 进程(线程)内访问临界资源的代码被成为临界区。、

#### 互斥量 mutex

因为时间片轮询机制和高级语言非原子操作导致的数据混乱问题。

**互斥锁：将加锁与解锁之间的代码进行锁定，让CPU执行完加锁与解锁之间的逻辑后再进行时间片轮询。**

```C++
#include <pthread.h>

// 声明一个互斥量    
pthread_mutex_t mtx;

// 初始化 （这里我认为 mtx 才是真正的【互斥锁】）
int pthread_mutex_init(&mtx, NULL);

// 加锁  
int pthread_mutex_lock(&mtx);

// 解锁 
int pthread_mutex_unlock(&mtx);

// 销毁
int pthread_mutex_destroy(&mtx);
```

#### pthread_mutex_init() 初始化互斥锁

`int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);`

- `mutex`：指向`pthread_mutex_t`类型的指针，用于存储互斥锁对象。

- `attr`：指向`pthread_mutexattr_t`类型的指针，表示互斥锁的属性。如果为`NULL`，则使用默认属性。

成功时返回`0`，失败时返回错误码并设置`errno`变量。

#### pthread_mutex_destroy() 销毁互斥锁

`int pthread_mutex_destroy(pthread_mutex_t *mutex);`销毁已初始化的互斥锁对象

#### pthread_mutex_lock()  加锁

`int pthread_mutex_lock(pthread_mutex_t *mutex);`

当一个线程调用`pthread_mutex_lock()`时

- 如果当前该互斥锁没有被其它线程持有，则该线程会获得该互斥锁，并将其标记为已被持有

- 如果该互斥锁已经被其它线程持有，则当前线程会被阻塞，直到该互斥锁被释放并重新尝试加锁。

#### pthread-mutex_unlock()  解锁

`int pthread_mutex_unlock(pthread_mutex_t *mutex);`

当一个线程调用`pthread_mutex_unlock()`时，它会释放该互斥锁

- 并且如果有其它线程正在等待该锁，则其中一个线程将被唤醒以继续执行。

- 如果当前没有线程解锁一个未被锁定的互斥锁，或者不是由该线程锁定的互斥锁，那么`pthread_mutex_unlock()`函数将返回`EPERM`错误，表示操作不允许

#### 条件变量 condition variable

让CPU时间片轮询机制变得可控。

因为CPU时间片轮询并不是顺序的，而是抢占式的。

**条件变量的存在可以让一个任务执行完成后指定下一个要执行的任务。**

这样执行顺序就能被确定下来，保证我们的逻辑正常。

### 同步

**同步是进程(线程)之间的直接制约关系。**

是指**多个相互合作的进程(线程)之间互相通信、互相等待**，这种相互制约的现象称为进程(线程)的同步。

### RWMutex 读写锁 

读写锁是一对互斥锁，分为读锁和写锁。

读锁和写锁互斥，让**一个线程在进行读操作时，不允许其他线程的写操作**，但是不影响其他线程的读操作。

**当一个线程在进行写操作时，不允许任何线程进行读操作或者写操作**。

```C++
#include <pthread.h>

// 定义读写锁
pthread_rwlock_t rwlock;

// 初始化
pthread_rwlock_init(&rwlock, NULL);  

// 加读锁
int pthread_rwlock_rdlock(&rwlock);

// 加写锁
int pthread_rwlock_wrlock(&rwlock);

// 释放锁
int pthread_rwlock_unlock(&rwlock);
```

#### pthread_rwlock_init() 初始化读写锁

`int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);`

- `rwlock`：指向`pthread_rwlock_t`类型的指针，用于存储读写锁对象。

- `attr`：指向`pthread_rwlockattr_t`类型的指针，表示读写锁的属性。如果为`NULL`，则使用默认属性。

成功时返回`0`，失败时返回错误码并设置`errno`变量。

#### pthread_rwlock_destroy() 销毁读写锁

`int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);` 销毁已初始化的读写锁对象

#### pthread_rwlock_rdlock() 加读锁

`int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);`用于获取读取锁`(pthread_rwlock_t)`上的**共享读取访问权限**。它允许多个线程同时读取共享资源，但不能写入它。

- 如果有线程已经持有写入锁，则其他线程将被阻塞直到写入锁被释放。

- 调用此函数时，如果另一个线程已经持有写入锁，则该线程将被阻塞，直到写入锁被释放

#### pthread_rwlock_wrlock() 加写锁

`int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);`用于获取写入锁`(pthread_rwlock_t)`上的**独占写入访问权限**。它阻止其他线程读取或写入共享资源，直到写入锁被释放。

- 如果有线程已经持有读取锁或写入锁，则其他线程将被阻塞直到写入锁被释放。

#### pthread_rwlock_unlock() 解锁

`int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);`

### Spinlock 自旋锁

一种基于忙等待的锁机制，它是一种轻量级的锁实现方式。

与传统的阻塞锁不同，**自旋锁在获取锁时不会主动阻塞线程，而是通过循环不断地尝试获取锁，直到成功获取为止。**

在多核处理器的环境下，自旋锁通常会使用底层的原子操作（如CAS）来实现。

当一个线程尝试获取自旋锁时，如果发现锁已经被其他线程持有，它会在循环中不断地检查锁的状态，而不是被挂起或阻塞。

这样做的目的是为了**避免线程被切换到其他任务上**，从而减少上下文切换的开销。

- 1.锁的保持时间很短：如果临界区的代码执行时间很短，使用自旋锁可以避免线程切换的开销，从而提高性能。

- 2.并发冲突较少：自旋锁适用于并发冲突较少的情况。如果临界区的竞争激烈，自旋锁可能会导致大量的线程空转，浪费CPU资源。

- 3.不可阻塞：自旋锁要求获取锁的操作是非阻塞的，即不会引起线程的挂起或阻塞。

- 如果获取锁的操作可能会引起线程的阻塞，使用自旋锁就不合适，应该选择其他类型的锁。

#### pthread_spin_init() 初始化自旋锁

`int pthread_spin_init(pthread_spinlock_t *lock, int pshared);`

- `lock`：指向`pthread_spinlock_t`类型的指针，用于存储自旋锁对象。

- `pshared`：如果为`0`，则自旋锁是进程内的；如果为`1`，则自旋锁是进程间的。

在调用`pthread_spin_init`函数之前，必须先分配内存空间来存储自旋锁变量。与`pthread_rwlock_t`类似，需要在使用自旋锁前先进行初始化才能正确使用。

#### pthread_spin_destroy() 销毁自旋锁

`int pthread_spin_destroy(pthread_spinlock_t *lock);`

该函数确保在销毁自旋锁之前所有等待的线程都被解除阻塞并返回适当的错误码。如果自旋锁已经被销毁，则再次调用`pthread_spin_destroy`将导致未定义的行为。

#### pthread_spin_lock() 加锁

`int pthread_spin_lock(pthread_spinlock_t *lock);`

与`mutex`不同，自旋锁在获取锁时忙等待，即不断地检查锁状态是否可用，如果不可用则一直循环等待，直到锁可用。当锁被其他线程持有时，调用`pthread_spin_lock()`的线程将在自旋等待中消耗`CPU`时间，直到锁被释放并获取到锁。

#### pthread_spin_unlock() 解锁

`int pthread_spin_unlock(pthread_spinlock_t *lock);`

调用该函数可以使其他线程获取相应的锁来访问共享资源。与`mutex`不同，自旋锁在释放锁时并不会导致线程进入睡眠状态，而是立即释放锁并允许等待获取锁的线程快速地获取锁来访问共享资源，从而避免了线程进入和退出睡眠状态的额外开销。

### CASLock 原子锁

`CASLock`类是一个原子锁类，用于实现原子操作。**原子操作是一种不可中断的操作，它要么全部执行，要么全部不执行，不会出现部分执行的情况。**用于实现线程同步和互斥，以确保对共享资源的访问是安全的。

#### atomic_flag.clear()  重置原子标志位

`atomic_flag.clear()`是`C++`标准库中的一个原子操作函数，用于将给定的原子标志位`（atomic flag）`清除或重置为未设置状态。

#### lock() 加锁

**`std::atomic_flag_test_and_set_explicit()`**是C++标准库中的一个原子操作函数，用于测试给定的原子标志位（atomic flag）是否被设置，并在测试后将其设置为已设置状态。该函数接受一个指向原子标志位对象的指针作为参数，并返回一个布尔值，表示在调用函数前该标志位是否已经被设置。第二个可选参数order用于指定内存序，以控制原子操作的内存顺序和同步行为。通过循环等待实现了互斥锁的效果。

**`std::memory_order_acquire`**是C++中的一种内存序，用于指定原子操作的同步和内存顺序。具体来说，使用`std::memory_order_acquire`可以确保在当前线程获取锁之前，所有该线程之前发生的写操作都被完全同步到主内存中。这样可以防止编译器或硬件对写操作进行重排序或延迟，从而确保其他线程可以正确地读取共享资源的最新值。

```C++
void lock() {
    while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire)) ;
}
```

#### **atomic_flag_clear_explicit()**

**`std::atomic_flag_clear_explicit()`**是C++标准库中的一个原子操作函数，用于将给定的原子标志位（atomic flag）清除或重置为未设置状态。该函数接受一个指向原子标志位对象的指针作为参数，并使用可选的第二个参数order来指定内存序，以控制原子操作的同步和内存顺序。



# 协程模块

**协程是一种用户态的轻量级线程**

## ucontext_t 类

```C++
#include <ucontext.h>
typedef struct ucontext_t {
    struct ucontext_t* uc_link;
    sigset_t uc_sigmask;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
    ...
    void makecontext(ucontext_t* ucp, void (*func)(), int argc, ...);
    int swapcontext(ucontext_t* olducp, ucontext_t* newucp);
    int getcontext(ucontext_t* ucp);
    int setcontext(const ucontext_t* ucp);
};
```

### **成员变量**

- **`uc_link`**：**指向另一个 `ucontext_t` 结构体的指针**。如果当前上下文的执行完成后需要返回到另一个上下文，那么可以通过 `uc_link` 连接到那个上下文。

- **`uc_sigmask`**：该字段存储了**当前上下文的信号掩码（signal mask）**，即在该上下文中屏蔽的信号集。

- **`uc_stack`**：这是**与当前上下文相关联的栈信息**。该字段通过 `stack_t` 类型来表示栈的属性（例如栈的起始地址、大小等）。

- **`uc_mcontext`**：这个字段是 `mcontext_t` 类型，**保存了当前上下文的机器状态**（寄存器状态等）。

### **成员函数**

#### **makecontext**

    `void makecontext(ucontext_t* ucp, void (*func)(), int argc, ...);`

    - 初始化一个`ucontext_t`, func参数指明了该`context`的入口函数，argc为入口参数的个数，每个参数的类型必须是int类型。

    - 另外在`makecontext`前，一般需要显示的初始化栈信息以及信号掩码集同时也需要初始化`uc_link`，以便程序退出上下文后继续执行。

#### **swapcontext**

`int swapcontext(ucontext_t* olducp, ucontext_t* newucp);`

保存当前上下文（`olducp`）并切换到另一个上下文（`newucp`）。当目标上下文执行完成时，控制会返回到 `olducp` 所表示的上下文。

- **`olducp`**：指向当前上下文的指针。在调用 `swapcontext` 后，当前上下文会被保存到 `olducp` 指向的 `ucontext_t` 中。

- **`newucp`**：指向目标上下文的指针，切换到该上下文执行

#### **getcontext**

`int getcontext(ucontext_t* ucp);`

- **`ucp`**：指向 `ucontext_t` 结构体的指针，用于保存当前上下文的状态

- `getcontext` 函数用于获取当前的上下文，并将其保存在 `ucp` 指向的结构体中。此函数在一些上下文切换操作中会被使用。

#### **setcontext**

`int setcontext(const ucontext_t* ucp);`

- **`ucp`**：指向 `ucontext_t` 结构体的指针，用于指定要切换到的上下文

- `setcontext` 函数用于切换到指定的上下文（由 `ucp` 指向）。当切换到该上下文时，程序会继续从该上下文的保存状态恢复执行。

**注意**
setcontext执行成功不返回
getcontext执行成功返回0，若执行失败都返回-1。
若uc_link为NULL,执行完新的上下文之后程序结束。

# 协程调度模块

## use_caller

- **`use_caller = true`**  在主线程上创建调度协程

    - 其中main主线程上有 1.main的主协程，2.调度协程，3.任务子协程

- **`use_caller = false`** 新建一个调度线程

    - 调度线程的主协程为调度协程，调度线程上有 1.调度协程，2.任务子协程

# IO协程调度模块

## 位操作表达式

```C++
// 确保当前事件已在 events 中注册
SYLAR_ASSERT(events & event);  // 逻辑与
// 从当前事件集合中移除该事件 使其不再被监听
events = (Event)(events & ~event); 
```

- `events & event` 是一个位操作表达式，表示将 `events` 和 `event` 进行按位与运算（AND），如果结果为真，则说明 `events` 中确实包含 `event` 事件（也就是说，`event` 事件已经注册在 `events` 中）。

- `~event` 是对 `event` 进行按位取反（NOT）操作。然后 `events & ~event` 表示将 `events` 中所有与 `event` 对应位相匹配的事件去掉。实际上，它移除了 `event` 对应的那一位，其他事件保持不变。

## epoll机制

`epoll` 是 Linux 提供的一种高效的事件通知机制，用于处理大量并发的 I/O 操作，尤其在高性能网络编程中应用广泛。与传统的 `select` 和 `poll` 不同，`epoll` 能更好地应对大量文件描述符的监听，具有更高的性能。

`epoll` 提供了三个主要的操作：

1. **`epoll_create`**：创建一个 `epoll` 实例，用于监听事件。

2. **`epoll_ctl`**：用于控制 `epoll` 实例的行为，主要用来注册、修改或删除监听的文件描述符。

3. **`epoll_wait`**：等待事件的发生，并返回已经触发的事件。

### epoll_create

`int epoll_create(int size);`

创建一个epoll句柄，参数为epoll监听的fd的数量(只要大于0)，返回文件描述符

```C++
m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0); // 检查epoll_creat是否成功
```

### pipe

创建一个管道，并返回两个文件描述符，`m_tickleFds[0]` 和 `m_tickleFds[1]`。这两个文件描述符分别用于读和写操作。

```C++
// 创建管道 用于唤醒 epoll_wait
    /*
     * pipe的作用是用于唤醒epoll_wait，因为epoll_wait是阻塞的，
     * 如果没有事件发生，epoll_wait会一直阻塞，
     * 所以需要一个pipe来唤醒epoll_wait
     */
    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);
```

### epoll_event

epoll事件结构体

**常见事件类型**

- **`EPOLLIN`**：表示可以读取数据。

- **`EPOLLOUT`**：表示可以写入数据。

- **`EPOLLERR`**：表示文件描述符发生了错误。

- **`EPOLLHUP`**：表示文件描述符被挂起。

- **`EPOLLET`**：启用边缘触发模式，只有在状态改变时触发事件，而不是每次检查都触发。

- **`EPOLLONESHOT`**：事件触发一次后自动移除该事件。

```C++
    // 初始化 epoll 事件结构
    epoll_event event;
    // 初始化epoll事件，清空event结构内存
    memset(&event, 0, sizeof(epoll_event));
    // 设置监听事件类型为：读事件，边缘触发
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0]; //关联文件描述符 设置为管道pipe 的读端描述符

    // 设置管道读端为 非阻塞模式
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(!rt);

```

### epoll_ctl

`int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);`

用于控制epoll实例的行为，主要用来注册，修改，删除监听的文件描述符

- epfd 为文件描述符，`epoll_create`函数的返回值

- op 为操作类型

    - **`EPOLL_CTL_ADD`**：将文件描述符添加到epoll实例中。

    - **`EPOLL_CTL_MOD`**：修改已添加到epoll实例中的文件描述符的关注事件。

    - **`EPOLL_CTL_DEL`**：从epoll实例中删除文件描述符。

- fd 要控制的文件描述符

- event 指向epoll_event结构体的指针

```C++
// 添加管道的读事件到epoll监听中
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);
```

添加事件注册

```C++
// 若已经有注册的事件则为修改操作 若没有则添加操作
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    // 设置 epoll 事件，使用边缘触发 并保留保留原始事件
    epevent.events = EPOLLIN | fd_ctx->events | event;
    // 将fd_ctx 保存到data指针中
    epevent.data.ptr = fd_ctx;

    // 注册事件
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }
```

删除事件注册

```C++
// 删除指定事件，创建新的不包含删除事件的事件集
Event new_events = (Event)(fd_ctx->events & ~event);  // 逻辑与一个 非event
// 如果还有其他事件，那么就是修改已注册事件，否则就是删除事件
int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
// 创建 epoll_event 结构体
epoll_event epevent;
// 设置epoll事件，使用边缘触发模式 新的注册事件(只有在事件从无到有变化时，epoll返回该事件)
epevent.events = EPOLLET | new_events;
// 将 fd_ctx 保存到data指针中
epevent.data.ptr = fd_ctx;

// 注册事件
// 调用epoll_ctl删除事件 将更新的事件集 epevent 注册到 m_epfd 中
/**
 * @brief 从用户空间将epoll_event结构copy到内核空间
 * @parm m_epfd   epoll文件描述符
 * @parm op       决定是修改还是删除事件
 * @parm fd       要操作的文件描述符
 * @parm epevent  告诉内核需要监听的事件
 */
int rt = epoll_ctl(m_epfd, op, fd, &epevent);
```

删除所有已注册事件

```C++
// 删除操作
int op = EPOLL_CTL_DEL;
epoll_event epevent;
// 删除所有事件
epevent.events = 0;
// 将fd_ctx 保存到data指针中
epevent.data.ptr = fd_ctx;

// 注册事件
int rt = epoll_ctl(m_epfd, op, fd, &epevent);
```

### epoll_wait

`int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);`

阻塞地等待事件发生，返回已经触发的事件列表。

- maxevents 监听事件的最大数量

- timeout 超时时间，表示`epoll_wait`阻塞的最长时间

    - **-1**：表示一直阻塞，直到有事件发生。 

    - **0**：表示立即返回，不管有没有事件发生。 

    - **>0**：表示等待指定的时间（以毫秒为单位），如果在指定时间内没有事件发生，则返回。

```C++
// 阻塞在epoll_wait上，等待事件发生
    int rt = 0;
    do {
        // 默认超时时间为3秒
        static const int MAX_TIMEOUT = 3000;

        if(next_timeout != ~0ull) {
            next_timeout = (int) next_timeout > MAX_TIMEOUT
                    ? MAX_TIMEOUT : next_timeout;
        } else {
            next_timeout = MAX_TIMEOUT;
        }

        // 等待事件发生，返回发生事件数量，-1 出错， 0 超时
        rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);

        // 如果是中断，继续等待
        if(rt < 0 && errno == EINTR){
        } else { // 否则退出循环
            break;
        }
    } while(true);
```

### 触发方式

**水平触发（Level-Triggered，LT）**

- 默认模式，当文件描述符有数据可以读/写时，`epoll_wait` 会返回并触发事件。直到数据被完全处理，事件才会被移除。

**边缘触发（Edge-Triggered，ET）**

- 在事件状态发生变化时触发一次事件。如果不处理完数据，事件不会再次触发。因此，在边缘触发模式下，必须尽可能快地处理所有数据，避免漏掉事件。

## Socket相关API

### socket()

**`SOCKET socket (int af , int type , int protocol)`**  创建套接字

- **`af`** :（Adress Family） 指定通信协议族

    - `AF_INET`：IPv4 地址族。

    - `AF_INET6`：IPv6 地址族。

    - `AF_UNIX`：本地 UNIX 域套接字。

- **`type`**：指定套接字的类型。

    - `SOCK_STREAM`：面向连接的流套接字（如 TCP）。

    - `SOCK_DGRAM`：数据报套接字（如 UDP）

- **`protocol`**：指定协议，通常可以传入 `0`，让系统自动选择适合的协议（如 TCP 或 UDP）

- **返回值**：成功时返回套接字文件描述符 fd，失败时返回-1

### connect()

`int connect(int sockfd,struct sockaddr*serv_addr,int addrlen);`  建立TCP连接

- **`sockfd`**: 客户端的socket描述符 

- **`serv_addr`**: 服务器的socket地址 

- **`addrlen`**: socket地址的长度 

- **return**: 成功返回0 ， 错误返回SOCKET_ERROR

### inet_pton

`int inet_pton(int af, const char *src, void *dst);` IP地址转换函数

- af：地址族，例如 AF_INET 或 AF_INET6。 

- src：包含一个以数字和点可以表示的IP地址的字符串。 

- dst：指向将要填充的新分配的内存的指针，大小由地址族确定。

```C++
// 创建 socket
int sock = socket(AF_INET, SOCK_STREAM, 0);

// 设置为非阻塞模式
fcntl(sock, F_SETFL, O_NONBLOCK);

// 声明接口地址
sockaddr_in addr;
// 分配空间
memset(&addr, 0, sizeof(addr));
// 设置 组族
addr.sin_family = AF_INET;
// 设置端口
addr.sin_port = htons(80);
// 设置IP IP地址转化函数
inet_pton(AF_INET, "36.155.132.76", &addr.sin_addr.s_addr);

// 建立连接
if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
} else if(errno == EINPROGRESS) {
    SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
    sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ,[](){
        SYLAR_LOG_INFO(g_logger) << "read callback";
    });
    sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE,[](){
        SYLAR_LOG_INFO(g_logger) << "write callback";

        sylar::IOManager::GetThis()->cancleEvent(sock, sylar::IOManager::READ);
        close(sock);
    });
}
```

### fcntl()

**`fcntl(sock, F_SETFL, O_NONBLOCK);`** 修改文件描述符的属性

- **`sock`**：套接字的文件描述符，指向由 `socket()` 创建的套接字。

- `F_SETFL` 是一个命令，用于修改文件描述符的标志。

- `O_NONBLOCK` 是一个标志，表示将文件描述符设置为 **非阻塞模式**

# 定时器模块

## gettimeofday()

`int gettimeofday(struct timeval *tv, struct timezone *tz)`

获取系统时间

- **`tv`**：定义一个时间结构体，包含秒数部分和微秒部分

    - `tv_sec`：秒数部分（`long` 类型）

    - `tv_usec`：微秒数部分（`long` 类型）

- **`tz`**：时区信息结构体，包含西经分钟数 和 夏令时修正

    - `tz_minuteswest`  ：西经的分钟数 

    - `tz_dsttime`  ： 夏令时修正类型 

```C++
#include <sys/time.h> 
uint64_t GetCurrentMS(){
    struct timeval tv; // 定义一个 timeval 结构体，用于存储时间
    gettimeofday(&tv, nullptr); // 获取当前时间，存入 tv 结构体
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000; // 返回当前时间的毫秒值
}
```

## lower_bound()

`template< class ForwardIt, class T, class Compare >
ForwardIt lower_bound( ForwardIt first, ForwardIt last, const T& value, Compare comp );`

在有序范围内查找第一个不小于给定值的元素

- `first` 和 `last`：定义了一个有序范围 `[first, last)`，函数会在这个范围内进行查找。

- `value`：要查找的目标值。

- `comp`（可选）：自定义的比较函数，用于定义元素的比较规则。如果不提供，默认使用 `<` 运算符进行比较。

`auto it = m_timers.lower_bound(now_timer);`在有序的关联容器中查找第一个不小于给定参数的元素

**与 `upper_bound()` 的区别**：

- `lower_bound()` 返回第一个**不小于** `now_timer` 的元素。

- `upper_bound()` 返回第一个**大于** `now_timer` 的元素

## OnTimer

```C++
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}
```

**在定时器触发时，检查某个资源是否仍然存在，如果存在则执行回调逻辑**

- `weak_cond` 是一个弱引用指针（不会增加引用计数，不会影响资源生命周期)

- `weak_cond.lock()`：尝试将 `weak_ptr` 提升为 `shared_ptr`。如果资源仍然存在（即引用计数 > 0），则返回一个有效的 `shared_ptr`；否则返回空的 `shared_ptr`

# Hook模块







# Address模块

## 内存操作相关函数

### memcmp()

`int memcmp(const void* ptr1, const void* ptr2, size_t num);`

- 比较 `ptr1` 和 `ptr2` 内存区域中字节的大小 

- `num`：要比较的字节数

### memset()

`void* memset(void* ptr, int value, size_t num);`

将内存块的每一个字节设置为指定的值（常用于**初始化或清空内存**）

- **`ptr`**：指向要设置值的内存区域的指针。

- **`value`**：要设置的值，类型为 `int`，但会被转换为 `unsigned char` 进行填充。

- **`num`**：要设置的字节数

- **return** : 返回传入的内存地址指针

```C++
memset(&m_addr, 0, sizeof(m_addr));
memset(buffer, 0, sizeof(buffer));  // 清空 buffer 数组
```

### memcpy()

`void* memcpy(void* dest, const void* src, size_t n);`

C 和 C++ 标准库中用于内存拷贝的函数

将一块内存的内容复制到另一块内存中

- **dest**：目标内存的起始地址，数据将被复制到该地址。

- **src**：源内存的起始地址，数据将从该地址复制过来。

- **n**：要复制的字节数，表示要从源地址复制多少字节到目标地址

```C++
memcpy(m_addr.sun_path, path.c_str(), m_length);
```

### memctr()

`void* memchr(const void* ptr, int value, size_t num);`  在一段内存中查找指定的字节

- **`ptr`**：指向要搜索的内存区域的指针。

- **`value`**：要查找的字节值（按字符处理）。

- **`num`**：要搜索的字节数，即从 `ptr` 开始的内存区域的长度

```C++
const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
// 在一个包含 IPv6 地址的字符串中找到 ] 的位置
```

## offsetof 宏

- `offsetof` 是一个宏，用来计算结构体成员相对于结构体起始位置的字节偏移量

- `offsetof(type, member)` 会返回成员 `member` 在结构体 `type` 中的字节偏移量

## Unix Socket

Unix Socket 是一种在 Unix 和类 Unix 系统中使用的通信机制，也称为 Unix 域套接字（Unix Domain Socket）。它提供了一种进程间通信（IPC）机制，允许**同一台计算机上的不同进程之间交换数据**。

与常规的网络套接字（如 TCP/IP 套接字）不同，Unix 套接字不依赖于网络协议栈，而是通过文件系统来实现进程间的通信。它们**使用文件系统中的路径来标识通信端点**，而不是 IP 地址和端口号。

### Unix Socket 的特点：

1. **文件系统路径**：Unix 套接字通过文件系统路径来标识（如 `/tmp/socket_file`）。这意味着它们只在本地系统内有效，不能跨越网络。

2. **速度**：由于不涉及网络协议栈的开销，Unix 套接字通信速度比 TCP/IP 更快。

3. **安全性**：使用文件权限来控制对 Unix 套接字的访问，因此可以基于文件系统权限控制访问。

4. **支持双向通信**：Unix 套接字支持流式（字节流）和数据报（数据块）两种通信方式，类似于 TCP 套接字。

5. **适用于本地进程间通信**：Unix 套接字仅用于同一台机器上的进程间通信。它非常适合本地进程间交换大量数据。

### 常见的 Unix 套接字类型：

- **SOCK_STREAM**：字节流类型，类似于 TCP 套接字，提供可靠的双向通信。

- **SOCK_DGRAM**：数据报类型，类似于 UDP 套接字，提供不可靠的数据传输。

## CreateMask()函数

```C++
template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof (T) * 8 - bits)) - 1;
}
```

- **`(sizeof (T) * 8`**   获取类型T的字节数再 ×8 获取类型T对应的比特数

    - `T` 是 `uint32_t` 类型，它占 4 个字节，即 `32` 位

- **`1 << (sizeof(T) * 8 - bits)`**  将数字 `1` 左移 `(sizeof(T) * 8 - bits)` 位

    - 如果 `T` 是 `uint32_t`（32 位），`bits` 是 `24`，那么左移操作就是 `1 << (32 - 24)`，即 `1 << 8`，结果为 `256`

- **`-1`**   将上一步的结果减 1，转换为全是 1 的二进制数

    - 比如，`256`（`1 << 8`）的二进制表示是 `100000000`，减去 1 后变为 `11111111`

- **result** 

    - 最终的结果是一个掩码，它的高位为 0，低位为 1（根据 `bits` 的值决定多少位是 1，剩余的位是 0）

## 条件编译优化

```C++
/**
 * @brief 优化条件编译的性能
 * @details  __builtin_expect 内建函数来告诉编译器，该条件大概率为真，从而优化代码生成
 */
#if defined __GNUC__ || defined __llvm__   // 针对GCC 或 LLVM 编译器
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
# define SYLAR_LIKELY(x)    __builtin_expect(!!(x), 1)
/// UNLIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
# define SYLAR_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#else
#define SYLAR_LIKELY(x)   (x)
#define SYLAR_UNLIKELY(x)   (x)
#endif
```

**`__builtin_expect`**：这是 GCC 和 LLVM 提供的一个编译器内建函数，用于帮助编译器优化条件判断。它通过提示条件的成立概率来指导编译器优化分支预测。

- **`__builtin_expect(!!(x), 1)`**：表示条件 `x` **成立**的可能性较高（类似于 `SYLAR_LIKELY(x)`）。

- **`__builtin_expect(!!(x), 0)`**：表示条件 `x` **不成立**的可能性较高（类似于 `SYLAR_UNLIKELY(x)`）。

```C++
if (SYLAR_LIKELY(x > 0)) {
    // 条件大概率成立时执行的代码
} else {
    // 条件大概率不成立时执行的代码
}
if (SYLAR_UNLIKELY(x < 0)) {
    // 条件大概率不成立时执行的代码
}
```

# Socket模块



## 标准库套接字API

POSIX 标准的套接字 API（如 `socket`, `bind`, `listen`, `accept` 等）。这些函数一般用于处理 TCP/IP 协议栈中的连接、监听、发送和接收数据等操作。

### 1. **socket()**

**`int socket(int domain, int type, int protocol);`**

创建一个套接字，用于网络通信。它返回一个套接字描述符，后续操作都需要通过该描述符。

- `domain`: 地址族，通常是 `AF_INET`（IPv4）或 `AF_INET6`（IPv6）。

- `type`: 套接字类型，通常是 `SOCK_STREAM`（TCP）或 `SOCK_DGRAM`（UDP）。

- `protocol`: 协议，通常为 0，表示默认协议。

### 2. **bind()**

**`int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`**

将创建的套接字绑定到本地地址（IP 和端口）。

- `sockfd`: 套接字描述符。

- `addr`: 地址结构体，通常是 `sockaddr_in` 或 `sockaddr_in6`。

- `addrlen`: 地址结构体的长度。

### 3. **listen()**

**`int listen(int sockfd, int backlog);`**

将服务器套接字设置为监听状态，准备接受连接请求。

- `sockfd`: 套接字描述符。

- `backlog`: 等待队列的最大长度。

### 4. **accept()**

**`int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);`**

从监听套接字中接受一个连接，并返回一个新的套接字，用于与客户端通信。

- `sockfd`: 监听套接字描述符。

- `addr`: 客户端地址信息。

- `addrlen`: 地址信息的长度。

### 5. **connect()**

**`int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`**

客户端调用，连接到远程服务器。

- `sockfd`: 套接字描述符。

- `addr`: 远程服务器的地址。

- `addrlen`: 地址的长度。

### 6. **send()**

**`ssize_t send(int sockfd, const void *buf, size_t len, int flags);`**

发送数据到连接的远程主机（客户端或服务器）。

- `sockfd`: 套接字描述符。

- `buf`: 数据缓冲区。

- `len`: 发送的数据长度。

- `flags`: 控制标志，通常为 0。

### 7. **recv()**

**`ssize_t recv(int sockfd, void *buf, size_t len, int flags);`**

从连接的远程主机接收数据。

- `sockfd`: 套接字描述符。

- `buf`: 数据缓冲区。

- `len`: 要接收的最大字节数。

- `flags`: 控制标志，通常为 0。

### 8. **close()**

**`int close(int sockfd);`**

关闭套接字，释放资源。

- `sockfd`: 套接字描述符。

## 网络编程流程

**服务器端：**

1. 使用 `socket()` 创建套接字。

2. 使用 `bind()` 将套接字绑定到指定的 IP 地址和端口。

3. 使用 `listen()` 使服务器开始监听客户端的连接请求。

4. 使用 `accept()` 接受客户端连接。

5. 使用 `recv()` 接收客户端发送的数据。

6. 最后，通过 `close()` 关闭连接和服务器套接字。

**客户端：**

1. 使用 `socket()` 创建套接字。

2. 使用 `inet_pton()` 转换服务器 IP 地址。

3. 使用 `connect()` 连接到服务器。

4. 使用 `send()` 发送数据。

5. 使用 `close()` 关闭套接字。


# 序列化模块
