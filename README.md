# Sylar-leo

---
```
- 学习参考
    - https://github.com/sylar-yin/sylar/tree/master
    - https://blog.csdn.net/qq_35099224/category_12613947.html
    - https://blog.beanljun.top/tags/C-C/
    - https://chatgpt.com/
- 相关知识点笔记
    - https://github.com/Lwaiz/sylar_leo/blob/master/Sylar_note.md
```

```
**2025.1.6**   Log系统

**2025.1.15**  配置系统

**2025.2.5**   线程模块

**2025.2.9**   协程模块
```

---

# 开发环境

```powershell
wsl2 ---- Ubuntu-22.04

gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0

g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0

cmake version 3.22.1
```


---

# Logger模块

---

## 功能特点

1. **灵活的日志级别**：
- 支持多种日志级别：DEBUG、INFO、WARN、ERROR 和 FATAL。
- 可根据严重性过滤日志。

2. **基于流的日志记录**：
- 提供基于流语法的日志宏，例如：`SYLAR_LOG_DEBUG(logger) << "消息内容";`。

3. **格式化日志记录**：
- 支持类似 `printf` 风格的格式化日志宏，例如：`SYLAR_LOG_FMT_INFO(logger, "值: %d", value);`。

4. **可扩展的日志格式化**：
- 支持基于模式的日志格式化。
- 常用模式包括时间戳、线程 ID、日志级别、文件名和行号等。

5. **线程与协程支持**：
- 捕获线程与协程信息，增强调试与追踪能力。

6. **动态日志配置**：
- 可配置日志级别和输出位置。
- 支持将日志写入文件或标准输出。

---

## 架构
### 1. 核心类
![img.png](readme_img/img.png)
![img.png](readme_img/log.png)
#### LogLevel 类
**定义日志级别**

包含 UNKNOW, DEBUG, INFO, WARN, ERROR, FATAL共五个级别 

通过 ToString函数 和 FromString函数实现了日志级别和文本的相互转换
（具体实现使用了宏定义）

#### LogEvent 类
**日志事件类**

包含一条日志记录的信息，包含日志时间、线程信息、日志级别、内容、代码位置等内容

#### LogEventWrap 类

为 LogEvent 提供包装器，简化宏中日志事件的管理

#### LogFormatter 类
**日志格式器**
*  %m 消息
*  %p 日志级别
*  %r 累计毫秒数
*  %c 日志名称
*  %t 线程id
*  %n 换行
*  %d 时间
*  %f 文件名
*  %l 行号
*  %T 制表符
*  %F 协程id
*  %N 线程名称

**默认格式模板**
*     "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"

format函数 返回格式化日志文本

#### FormatItem 类
* @brief 日志内容项格式化类
    *   每个 FormatItem 表示一个日志格式化标记的解析结果，例如 %m 或 %p
    *   继承 FormatItem，可以为不同的标记（如 %m 消息）自定义解析和格式化逻辑
**init 初始化函数**
解析日志格式化字符串模式（m_pattern）并将解析结果存储到一个vec中
* @example "%d{%Y-%m-%d %H:%M:%S} [%p] %c: %m%n"
  * %d 表示日期时间，%p 表示日志级别，%c 表示日志名称，%m 表示消息内容，%n 表示换行符
  * 遇到 %d：检测到 {%Y-%m-%d %H:%M:%S} 为参数，解析为 ("d", "{%Y-%m-%d %H:%M:%S}", 1)。
  * 遇到 [: 普通字符，解析为 ("[", "", 0)。
  * 遇到 %p：无参数，解析为 ("p", "", 1)。
  * 遇到 %c：无参数，解析为 ("c", "", 1)。
  * 遇到 %m：无参数，解析为 ("m", "", 1)。
  * 遇到 %n：无参数，解析为 ("n", "", 1)。

**init 函数具体实现**
1. 初始化 定义容器与变量
2. 遍历格式化模式 m_pattern
3. 格式化解析字符串部分
4. 根据解析结果更新 vec
5. 处理剩余普通字符串
6. 格式化项创建函数映射
7. 遍历 vec 生成格式化项

#### LogAppender 类
**日志输出目标类: 控制台 / 文件**



#### StdOutLogAppender 类
输出到控制台的Appender

#### FileLogAppender 类
输出到文件的Appender

#### Logger 类
**日志器**

### 2. **日志宏**

#### **基于流的日志记录**
```cpp
SYLAR_LOG_DEBUG(logger) << "调试信息";
SYLAR_LOG_INFO(logger) << "普通信息";
SYLAR_LOG_ERROR(logger) << "错误信息";
```

#### **格式化日志记录**
```cpp
SYLAR_LOG_FMT_DEBUG(logger, "值: %d", value);
SYLAR_LOG_FMT_INFO(logger, "用户: %s", username.c_str());
```

### 3. **Logger**
管理日志事件，并将其发送到适当的输出目标（如控制台、文件）。

---

## 设置与使用

### 1. **集成**
在项目中包含头文件：
```cpp
#include "sylar/log.h"
```

### 2. **创建 Logger**
```cpp
sylar::Logger::ptr logger = std::make_shared<sylar::Logger>();
logger->setLevel(sylar::LogLevel::DEBUG);
```

### 3. **记录日志**
基于流的日志记录：
```cpp
SYLAR_LOG_INFO(logger) << "应用启动";
```

格式化日志记录：
```cpp
SYLAR_LOG_FMT_ERROR(logger, "错误代码: %d", errorCode);
```

### 4. **自定义日志格式**
定义自定义的日志消息格式：
```cpp
logger->setFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%m%n");
```

---

## 示例

### 示例 1: 基础日志记录
```cpp
#include "sylar/log.h"

int main() {
    sylar::Logger::ptr logger = std::make_shared<sylar::Logger>();
    logger->setLevel(sylar::LogLevel::INFO);

    SYLAR_LOG_INFO(logger) << "这是普通信息。";
    SYLAR_LOG_ERROR(logger) << "这是错误信息。";

    return 0;
}
```

### 示例 2: 高级格式化
```cpp
#include "sylar/log.h"

int main() {
    sylar::Logger::ptr logger = std::make_shared<sylar::Logger>();
    logger->setFormatter("%d{%H:%M:%S} %p %m%n");

    SYLAR_LOG_WARN(logger) << "带有自定义格式的警告！";

    return 0;
}
```

---

---

# 配置系统

---

## 功能特性
- **约定由于配置**:采用约定由于配置的思想。定义即可使用。不需要单独去解析。支持变更通知功能。使用YAML文件做为配置内容。
- **支持多种类型**：支持常见的基础类型和容器类型。
- **动态管理**：通过统一的接口动态获取和设置配置参数。
- **类型安全**：利用模板机制确保类型转换安全。
- **序列化与反序列化**：提供参数值与 YAML 格式字符串之间的互相转换。
- **日志支持**：对于异常情况提供详细的日志记录。

## 系统架构
- **ConfigVarBase**

  ```配置变量的基类，其主要是虚方法，定义了配置项中共有的成员和方法，其具体实验由基类的具体子类负责实现。其中重点是toString()方法和fromString()方法，其分别负责将配置信息转化字符串和从字符串中解析出配置。```
- **ConfigVar**

  ```配置参数模板子类，并且是模板类，其具有三个模板参数，分别是配置项类型模板T、仿函数FromStr和仿函数Tostr。ConfigVar类在ConfigVarBase的基础上，增加了AddListener方法和delListener等方法，用于增删配置变更回调函数。```
- **Config**

  ```ConfigVar的管理类，其提供便捷的方法创建、访问ConfigVar，其主要方法未Lookup方法，可根据配置的名称查询配置项，如果查询时未找到对应的配置则新建一个配置项。其还有从配置文件中读取配置、遍历所有配置项等方法。其中，Config中的方法都是static方法，确保全局只有一个实例。```

### 核心组件
![img.png](readme_img/config.png)
#### 1. `ConfigVarBase`
**作用**：配置变量的基类，定义了所有配置项的基本属性和接口。
- **主要成员变量**：
  - `m_name`：配置参数的名称。
  - `m_description`：配置参数的描述。
- **主要接口**：
  - `toString()`：将配置参数值序列化为字符串。
  - `fromString()`：从字符串反序列化为配置参数值。

#### 2. `ConfigVar`
**作用**：配置参数模板子类，用于存储具体类型的配置变量，继承自 `ConfigVarBase`。
- **模板参数**：
  - `T`：配置参数的类型。
  - `FromStr`：从字符串转换为 `T` 类型的仿函数。
  - `ToStr`：从 `T` 类型转换为字符串的仿函数。
  - `addListener`、`delListener`、`clearListener`等方法用于设置配置值改变时的回调函数
- **主要功能**：
  - 保存和操作具体类型的配置变量值。
  - 利用模板特化支持容器类型。

#### 3. `LexicalCast`
**作用**：通用的类型转换工具。
- **主要功能**：
  - 基本类型之间的转换（如 `int` 转 `std::string`）。
  - 支持容器类型与 YAML 字符串之间的相互转换。
- **特化实现**：支持 vector,list,set,unordered_set,map,unordered_map
  - `LexicalCast<std::string, std::vector<T>>`：从 YAML 字符串转换为 `std::vector<T>`。
  - `LexicalCast<std::vector<T>, std::string>`：将 `std::vector<T>` 转换为 YAML 字符串。
  - `LexicalCast<std::list<T>, std::string>`：将 `std::list<T>` 转换为 YAML 字符串。 
  - `LexicalCast<std::string, std::set<T>>`：将 YAML 字符串转换为 `std::set<T>`。 
  - `LexicalCast<std::set<T>, std::string>`：将 `std::set<T>` 转换为 YAML 字符串。 
  - `LexicalCast<std::string, std::unordered_set<T>>`：将 YAML 字符串转换为 `std::unordered_set<T>`。 
  - `LexicalCast<std::unordered_set<T>, std::string>`：将 `std::unordered_set<T>` 转换为 YAML 字符串。 
  - `LexicalCast<std::string, std::map<std::string, T>>`：将 YAML 字符串转换为 `std::map<std::string, T>`。 
  - `LexicalCast<std::map<std::string, T>, std::string>`：将 `std::map<std::string, T>` 转换为 YAML 字符串。 
  - `LexicalCast<std::string, std::unordered_map<std::string, T>>`：将 YAML 字符串转换为 `std::unordered_map<std::string, T>`。 
  - `LexicalCast<std::unordered_map<std::string, T>, std::string>`：将 `std::unordered_map<std::string, T>` 转换为 YAML 字符串。
#### 4. `Config`
**作用**：配置管理类，提供全局统一的配置操作接口。
- **主要成员变量**：
  - `s_datas`：保存所有配置参数的映射表（键为参数名，值为配置变量指针）。
  - 提供了`Lookup`方法来根据配置名称查找配置项（并返回指定类型的配置项），
      - 如果找不到则可以创建新的配置项。
  - `LoadFromYaml`方法用于从YAML节点加载配置
- **主要功能**：
  - 动态查找和创建配置参数。
  - 从 YAML 文件加载配置。

### 配置加载流程
1. **定义配置变量**：使用 `Config::Lookup` 接口定义配置项，并设置默认值和描述。
2. **加载 YAML 文件**：通过 `Config::LoadFromYaml` 接口加载 YAML 格式的配置文件。
3. **访问配置参数**：通过 `Config::LookupBase` 或特定类型的 `Config::Lookup` 方法访问配置参数。
4. **动态修改配置参数**：在运行时修改配置值，通过 `toString` 和 `fromString` 实现参数的序列化与反序列化。

### 配置参数生命周期
1. **创建**：配置参数首次通过 `Config::Lookup` 定义并添加到全局映射表。
2. **管理**：所有配置项存储于 `Config` 类的 `s_datas` 静态变量中，统一管理。
3. **销毁**：程序退出时自动释放资源。

## 示例代码
### 定义配置参数
```cpp
#include "config.h"

// 定义一个整型配置变量
sylar::ConfigVar<int>::ptr g_int_config =
    sylar::Config::Lookup<int>("system.port", 8080, "Server port");

// 定义一个字符串向量配置变量
sylar::ConfigVar<std::vector<std::string>>::ptr g_vec_config =
    sylar::Config::Lookup<std::vector<std::string>>("system.hosts", {"127.0.0.1"}, "Server hosts");
```

### 加载配置文件
```cpp
#include "yaml-cpp/yaml.h"
#include "config.h"

void LoadConfig() {
    YAML::Node root = YAML::LoadFile("config.yml");
    sylar::Config::LoadFromYaml(root);
}
```

### 访问配置参数
```cpp
int port = g_int_config->getValue();
std::vector<std::string> hosts = g_vec_config->getValue();
```

### 动态修改参数
```cpp
g_int_config->setValue(9090);
std::cout << g_int_config->toString() << std::endl;
```

## 日志与异常处理
- 在配置加载和类型转换失败时，系统会记录详细的错误日志，便于调试。
- 配置参数的名称需要符合正则表达式 `[a-z0-9_.]+`，否则会抛出异常。

## 依赖
- `boost`：用于类型转换。
- `yaml-cpp`：解析 YAML 配置文件。

## 配置系统整合日志系统

---

---
# 线程模块 

---

## 功能特性

该模块提供了一个 **线程管理和同步机制** 的实现。它包括以下主要功能：
- 线程的创建与管理（Thread 类）
- 各种同步原语，包括互斥锁、读写锁、信号量等（Mutex、RWMutex、Semaphore 等类）
- 线程同步的模板实现（ScopedLockImpl、ReadScopedLockImpl、WriteScopedLockImpl 等类）

这些功能实现了多线程并发控制与线程间资源共享时的同步操作。

## 核心组件

```
Thread
├── Thread        // 线程类，管理线程创建、启动、终止    基于 pthread 实现
├── Mutex         // 互斥量类，防止线程间资源竞争       基于 pthread_mutex_t 实现
├── RWMutex       // 读写锁类，优化读操作的性能         基于 pthread_rwlock_t 实现
├── Semaphore     // 信号量类，控制并发数              基于 sem_t 实现
├── Spinlock      // 自旋锁类，针对高并发低延迟场景     基于pthread_spinlock_t实现
└── CASLock       // 基于原子操作的锁                 基于std::atomic_flag实现
```

### 1. Thread 类
Thread 类是一个封装线程操作的类，主要用于创建和管理线程。它提供了对线程执行、名称、ID等信息的访问，以及等待线程结束的功能。该类使用了 pthread 库来实现线程的创建和管理。

**线程构造和执行：**

```
通过传入一个线程执行回调函数（cb）和线程名称（name）来创建线程。
线程启动时，会执行传入的回调函数 cb。 
```

**线程信息访问：**

- getId()：获取线程的 ID（pid_t 类型）。
- getName()：获取线程的名称。

**等待线程执行完毕：**

- join()：等待线程执行完毕，类似于 pthread_join()，确保线程退出。

**静态线程操作：**

- GetThis()：获取当前线程的指针。
- GetName() 和 SetName()：获取和设置当前线程的名称。

**线程入口函数：**

- run()：作为线程的入口函数，调用传入的回调函数来执行线程任务

### 2. Semaphore 类
   Semaphore 是 **信号量类** ，通常用于控制对共享资源的并发访问。在这个类中：

- wait() 方法会使线程进入等待状态（即等待信号量）。
- notify() 方法会释放信号量，通知一个等待的线程。
信号量是通过 sem_t 类型实现的，sem_t 是 POSIX 标准定义的信号量类型，通常用于进程间或线程间的同步。

### 3. ScopedLockImpl 模板类
   ScopedLockImpl 是一个局部锁（RAII 锁）模板类，在作用域内自动加锁，离开作用域时自动解锁。它有以下功能：

- 在构造函数中调用 m_mutex.lock() 来加锁。
- 在析构函数中调用 unlock() 来解锁，确保资源释放。

### 4. ReadScopedLockImpl 模板类
   ReadScopedLockImpl 是一个局部读锁模板类，用于读写锁（pthread_rwlock_t）。它的功能与 ScopedLockImpl 类似，只不过是加读锁而不是普通的互斥锁：

- 在构造函数中调用 m_mutex.rdlock() 来加读锁。
- 在析构函数中调用 unlock() 来解锁。

### 5.WriteScopedLockImpl 模板类
   WriteScopedLockImpl 是一个局部写锁模板类，类似于 ReadScopedLockImpl，但是用于加写锁：

- 在构造函数中调用 m_mutex.wrlock() 来加写锁。
- 在析构函数中调用 unlock() 来解锁。

### 6. Mutex 类
   Mutex 类是一个基本的 **互斥锁** 实现，内部使用 pthread_mutex_t 来加锁和解锁：

    互斥锁是一种同步原语，用于控制对共享资源的访问。互斥锁可以确保在任何时候只有一个线程
    可以访问共享资源，从而避免竞争条件和数据竞争。

- lock()：调用 pthread_mutex_lock() 来加锁。
- unlock()：调用 pthread_mutex_unlock() 来解锁。
Mutex 类同时定义了一个 Lock 类型，表示局部锁。

### 7. NullMutex 类
   NullMutex 类是一个空锁实现，通常用于调试，避免在某些情况下加锁的开销：

- lock() 和 unlock() 方法为空实现，不做任何操作。

### 8. RWMutex 类
   RWMutex 是一个读写锁实现，**用于支持多个线程并发读取，或者一个线程独占写入**：

- rdlock()：加读锁，允许多个线程并发读取。
- wrlock()：加写锁，独占资源。
- unlock()：释放锁。

RWMutex 类同时定义了 ReadLock 和 WriteLock 类型，分别表示局部的读锁和写锁。

### 9. NullRWMutex（空读写锁）
类似 NullMutex，但用于读写锁的场景。

- rdlock()、wrlock() 和 unlock() 都为空实现。

### 10. Spinlock（自旋锁）
自旋锁是一种高效的锁机制，当线程无法获取锁时会反复检查锁是否可用，而不是将自己挂起。

    与mutex不同，自旋锁不会使线程进入睡眠状态，而是在获取锁时进行忙等待，直到锁可用。
    当锁被释放时，等待获取锁的线程将立即获取锁，从而避免了线程进入和退出睡眠状态的额外开销。

- lock()：尝试获取自旋锁，若无法获取则反复自旋。
- unlock()：释放自旋锁， 可以使其他线程获取相应的锁来访问共享资源。

### 11. CASLock（原子锁） 
使用原子操作实现的 **原子锁** ，通常用于高并发场景，避免线程上下文切换的开销。

    原子操作是一种不可中断的操作，它要么全部执行，要么全部不执行，不会出现部分执行的情况。

- lock()：使用原子操作尝试获取锁，直到获取成功。
- unlock()：通过原子操作释放锁。