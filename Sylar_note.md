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
        return #name; \
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



