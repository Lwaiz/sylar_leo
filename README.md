# Sylar-leo

---

**2025.1.6**

Log系统
- 学习参考
    - https://github.com/sylar-yin/sylar/tree/master
    - https://blog.csdn.net/qq_35099224/category_12613947.html
    - https://chatgpt.com/

**2025.1.15**

配置系统





---

# 开发环境

wsl2 ---- Ubuntu-22.04

gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0

g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0

cmake version 3.22.1

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
- **支持多种类型**：支持常见的基础类型和容器类型（如 `int`, `double`, `std::string`, `std::vector<T>`）。
- **动态管理**：通过统一的接口动态获取和设置配置参数。
- **类型安全**：利用模板机制确保类型转换安全。
- **序列化与反序列化**：提供参数值与 YAML 格式字符串之间的互相转换。
- **日志支持**：对于异常情况提供详细的日志记录。

## 系统架构

### 核心组件

#### 1. `ConfigVarBase`
**作用**：配置变量的基类，定义了所有配置项的基本属性和接口。
- **主要成员变量**：
  - `m_name`：配置参数的名称。
  - `m_description`：配置参数的描述。
- **主要接口**：
  - `toString()`：将配置参数值序列化为字符串。
  - `fromString()`：从字符串反序列化为配置参数值。

#### 2. `ConfigVar`
**作用**：模板类，用于存储具体类型的配置变量，继承自 `ConfigVarBase`。
- **模板参数**：
  - `T`：配置参数的类型。
  - `FromStr`：从字符串转换为 `T` 类型的仿函数。
  - `ToStr`：从 `T` 类型转换为字符串的仿函数。
- **主要功能**：
  - 保存和操作具体类型的配置变量值。
  - 利用模板特化支持容器类型（如 `std::vector<T>`）。

#### 3. `LexicalCast`
**作用**：通用的类型转换工具。
- **主要功能**：
  - 基本类型之间的转换（如 `int` 转 `std::string`）。
  - 支持容器类型与 YAML 字符串之间的相互转换。
- **特化实现**：
  - `LexicalCast<std::string, std::vector<T>>`：从 YAML 字符串转换为 `std::vector<T>`。
  - `LexicalCast<std::vector<T>, std::string>`：将 `std::vector<T>` 转换为 YAML 字符串。

#### 4. `Config`
**作用**：配置管理类，提供全局统一的配置操作接口。
- **主要成员变量**：
  - `s_datas`：保存所有配置参数的映射表（键为参数名，值为配置变量指针）。
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

