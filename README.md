# Logger模块
## LogLevel 类
**定义日志级别**

包含 UNKNOW, DEBUG, INFO, WARN, ERROR, FATAL共五个级别 

通过 ToString函数 和 FromString函数实现了日志级别和文本的相互转换
（具体实现使用了宏定义）

## LogEvent 类
**日志事件类**

包含一条日志记录的信息，包含日志时间、线程信息、日志级别、内容、代码位置等内容

## LogFormatter 类
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

### FormatItem 类
* @brief 日志内容项格式化类
    *   每个 FormatItem 表示一个日志格式化标记的解析结果，例如 %m 或 %p
    *   继承 FormatItem，可以为不同的标记（如 %m 消息）自定义解析和格式化逻辑
#### init 初始化函数
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

## LogAppender 类
**日志输出目标类: 控制台 / 文件**



### StdOutLogAppender 类
输出到控制台的Appender

### FileLogAppender 类
输出到文件的Appender

## Logger 类
**日志器**


