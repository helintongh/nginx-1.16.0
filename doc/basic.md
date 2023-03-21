# nginx源码安装

操作系统: ubuntu 22.04

依赖的软件库: libpcre3 libpcre3-dev openssl libssl-dev zlib1g-dev

其中，Nginx 如果要支持正则表达式，那么需要 libpcre3 和 libpcre3-dev；Nginx如果要支持加密功能，那么需要 openssl和libssl-dev；Nginx如果要支持压缩功能，那么需要zlib1g-dev。

用到的工具：curl unzip

CURL是一款用来模拟 HTTP 请求的工具，可以使用它来检查Web服务功能是否正常，查看请求和回复详情，下载文件等等。

unzip用于解压.zip格式的压缩包。

下载nginx-1.18.0的源代码

```shell
cd nginx-1.18.0 # 进入代码路径
# 执行 configure，目的是为了检查系统支持哪些特性，生成符合系统环境的 Makefile
./configure
# 编译并安装
make && sudo make install
```

此时，Nginx被安装在`/usr/local/nginx/sbin`目录下，其配置文件路径是 `/usr/local/nginx/conf/nginx.conf`。

下面简要介绍一下 nginx.conf 中的一些配置项的含义：

!!! 基本配置项

    worker_processes 1; # 表示 Nginx 子进程（即 worker 进程）的个数
    error_log logs/error.log notice; #表示记录错误日志的路径和级别，路径为 /usr/local/nginx/logs
    worker_connections 1024; # 这就是 Nginx 为什么很高效一节中提到的连接池的大小
    sendfile on; # 默认启用 sendfile 功能
    listen 80; # 表示 worker 进程监听 80 端口
    server_name localhost; # 虚拟主机名，感兴趣的学员可以在网上查阅什么是 VHost 功能
    location / ... # 表示默认的 HTTP 请求（即 URL 不带任何参数）映射的路径下的一些配置
    error_page ... # 表示 Nginx 返回这些错误码时，返回由它指定的页面内容


最后，检查系统中是否有已经运行的 Nginx：

```shell
ps aux | grep nginx
```


执行以下命令停止Nginx：

kill掉nginx的master进程即可。

```shell
sudo kill pid_of_master_of_nginx
```

然后再执行命令：

```shell
ps aux | grep nginx
```

如果结果与上上个图类似，就可以运行我们自己编译的 Nginx 了：

```
sudo /usr/local/nginx/sbin/nginx
```

写一个简单的页面来验证 Nginx 是否正常运行：

## 添加页面文件

```shell
sudo vim /usr/local/nginx/html/hello.html
```

加入如下的内容：

```html
<html>
  <head>
    <title>Hello Nginx</title>
  </head>
  <body>
    <h1>Hello World</h1>
  </body>
</html>
```

通过浏览器访问主机:

```
http://localhost/hello.html
```

![](resource/hello_world.png)

## curl工具的基本使用

curl是用在命令行或者脚本中传输数据的工具，不仅如此，CURL还提供了编写程序的接口，以供用户编写自己的程序。CURL不仅支持HTTP协议系列，还支持FTP，IMAP，POP3，RTMP，SMB 等协议系列。

curl请求http服务器时会打印接收到的HTTP回复包的body段:

常用选项:

- `-v`: 打印详细的 HTTP 请求和回复的信息
- `--http1.0`: 强制发送 HTTP/1.0 请求
- `-o`: 将回复写入指定的文件
- `-H`: 指定自定义的 HTTP 头
- `-X`：指定 HTTP 请求的请求方法（method）
- `--limit-rate`：指定限制速率的大小
