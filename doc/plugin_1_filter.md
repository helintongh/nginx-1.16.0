# 开发一个HTTP过滤模块

开发一个模块，它的作用是在 HTTP 回复中添加一个自定义的 HTTP 头，如 Client-IP: 127.0.0.1

知识点：

- nginx发送http回复的流程
- nginx处理http回复的函数指针链表
- nginx表示tcp连接的结构体和要用到的成员
- nginx的链表数据结构
- 如何在HTTP回复中添加自定义的HTTP头

