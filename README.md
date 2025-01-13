# Logger模块
## LogLevel类
定义日志级别，包含 UNKNOW, DEBUG, INFO, WARN, ERROR, FATAL共五个级别 

通过 ToString函数 和 FromString函数实现了日志级别和文本的相互转换
（具体实现使用了宏定义）

## LogEvent 类
日志事件类 包含一条日志记录的信息，包含日志时间、线程信息、日志级别、内容、代码位置等内容


