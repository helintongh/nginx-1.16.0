### nginx源码安装

操作系统: ubuntu 22.04

依赖的软件库: libpcre3 libpcre3-dev openssl libssl-dev zlib1g-dev

其中，Nginx 如果要支持正则表达式，那么需要 libpcre3 和 libpcre3-dev；Nginx如果要支持加密功能，那么需要 openssl和libssl-dev；Nginx如果要支持压缩功能，那么需要zlib1g-dev。

用到的工具：curl unzip

CURL是一款用来模拟 HTTP 请求的工具，可以使用它来检查Web服务功能是否正常，查看请求和回复详情，下载文件等等。

unzip用于解压.zip格式的压缩包。

下载
