## nginx开发

nginx的调式是困难的因为它是事件驱动的服务器,那么采用更加先进的IDE必然可以更好的去进行nginx的开发,所以我决定做一套用clion做nginx二次开发的教程。

### clion基础快捷键

|快捷键|描述|
|------|------|
|F8|单步调试|
|ALT+SHIFT+F8|强制单步运行|
|F7|跳入函数|
|SHIFT+F8|跳出函数|
|F9|恢复到正常运行|

clion的断电管理和变量监控

鼠标点相应行左侧即可下断点,红点出现断点就生效了。

### 用clion优雅的调试nginx

clion调试nginx总共分4步: nginx的configure配置 -> clion导入 -> python脚本根据objs下的makefile修改clion生成的CMakeLists.txt -> 修改nginx配置然后调试

1. nginx源码下载解压

在编译源码之前,需要安装nginx的一些依赖。要安装如下软件

gcc,pcre,pcre-devel,zlib,zlib-devel,openssl,openssl-devel。

执行如下指令
```shell
./configure --prefix=/root/nginx-1.16.0
```
--prefix是指生成的文件夹。

执行成功如下图:
![执行成功返回](pic/01/01.png)

这会生成objs的src其中有非常重要的三个源文件`nginx_auto_config.h, nginx_auto_header.h, ngx_modules.h`。clion需要引入这几个文件。

2. 利用Clion从nginx源码根路径导入

导入过程如下图:

![导入过程](pic/01/02.png)

导入过后会自动勾选一部分代码然后clion会自动勾选一些代码。

![自动勾选](pic/01/03.png)

此时你可以通过如下图
![clion导入和makefile对比](pic/01/04.png)
对比makefile和CMakeLists的区别来修改clion自动生成的CMakeLists。因为clion默认是通过代码全部编译,这里和./configure的执行情况有所不同。所以可以写一个脚本来改造CMakeLists

3. 通过脚本修改clion生成的cmake

该脚本在 [makefile2cmakelist.py](https://github.com/helintongh/nginx-1.16.0/blob/master/src/py/makefile2cmakelist.py)
```shell
python makefile2cmakelist.py CMakeLists.txt objs/Makefile
```

此时就可用clion进行构建nginx了。直接看图

构建如下图即可
![build](pic/01/05.png)

清除生成的文件如下即可
![clean](pic/01/06.png)

4. 修改配置

此时能够编译通过生成可执行文件但是有可能无法运行(通过Clion Run来运行的话)。还有几个几个小步骤。

首先看下面的报错如图:

![log-error](pic/01/07.png)

上面是因为运行nginx的时候缺少logs文件夹,这时候需要手动`mkdir logs` 生成logs文件夹,就可运行了。如果是端口被占用那么`kill` 掉占用80端口的进程即可。

下面可运行后可以通过
```shell
ps -ef | grep nginx
```
执行结果如下图:
![ps结果](pic/01/08.png)
可以看到现在运行的nginx是带了1.16后缀的。

此时通过浏览器访问127.0.0.1会出现如下图:

![forbid](pic/01/08.png)

还需要修改nginx的配置才能更好的调式。
首先修改用户名,如下图:
![user](pic/01/09.png)
我这台计算机使用的是root用户所以改为root用户,这样浏览器访问127.0.0.1就如下图了。
![可访问](pic/01/10.png)

与此同时还有一个很重要的操作修改如下图:
![off](pic/01/11.png)

daemon和master_process,daemon off是让当前nginx不是后台运行(非守护进程),master_process off是让nginx不以master/worker方式工作让其变为只有一个进程方便调试。

### 编译不带后缀的nginx

依次执行如下语句:

```shell
tar -zxvf nginx-1.16.0.tar.gz # 解压nginx
mv nginx-1.16.0 nginx # 改名
cp -r nginx /usr/local/ # 放到默认路径下
cd /usr/local/nginx
./configure --prefix=/usr/local/nginx
```
然后用Clion导入/usr/local/nginx路径重复上述步骤即可。

### nginx调试实战

下面我将来调试nginx的http处理模块

src路径下找到http,在http找到`ngx_http_core_module.c`,找到函数ngx_http_handler
这个函数来接收用户请求的,用户请求一进来到了这个函数。打如下图的断点:
![断点](pic/01/12.png)
然后运行,主义运行不是普通模式而是调试模式(点右上角的小虫子或SHIFT+F9)

此时按F8(单步运行)也不能运行因为没有请求过来根本运行不到这段代码。

通过浏览器访问127.0.0.1的nginx页面会卡住,卡在了断点的地方。

Clion的调试框如下图:

![变量](pic/01/13.png)
可以看到请求进入后r中所有的请求信息,包括url等等信息。

然后按F8向下运行,再按多个F9直到没有效果(这个请求的处理结束了)。

再在浏览器中输入`http://127.0.0.1/index.html?qq=10000`
![r变量](pic/01/14.png) 
这样就看到了吧,nginx的接收请求的过程能完全被我们所把控。

### 开发一个模块的步骤

在nginx src(**不是objs路径下的src**)路径下,新建一个文件夹modules然后再新建一个文件夹ngx_http_hello_module,在该文件夹下新增config和ngx_http_hello_module.c两个文件。一个是模块的配置,一个是源文件。

新建后代码结构如下图:

![代码结构](pic/01/15.png)

注意代码结构每一个模块都新建一个文件夹。

先写配置文件config

第一项是插件名称,第二项是代表其为http的模块,第三项是指定源文件

```shell
ngx_addon_name=ngx_http_hello_module
HTTP_MODULES="$HTTP_MODULES ngx_http_hello_module"
NGX_ADDON_SRCS="$NGX_ADDON_SRCS $ngx_addon_dir/ngx_http_hello_module.c"
```
编译指令要改为(此步骤要在你自己的nginx路径下我的在/usr/local/nginx):
```shell
./configure --prefix=/usr/local/nginx --add-module=src/modules/ngx_http_hello_module
```
执行结果注意下图红框标识的
![添加模块](pic/01/16.png)
当出现红框的字段标识已经把模块新增进去了。

再执行上面的提到过的脚本 [makefile2cmakelist.py](https://github.com/helintongh/nginx-1.16.0/blob/master/src/py/makefile2cmakelist.py)

```shell
python makefile2cmakelist.py CMakeLists.txt objs/Makefile
```
执行该脚本后成功修改了CMakeLists.txt,就可以通过Clion把插件编译进去了。

![执行成功修改cmake](pic/01/17.png)

### nginx模块关键代码结构

看别人的源码是最好的学习方式。所以关注ngx_http_stub_status_module.c,这一个nginx的原生代码就能明白。(路径在nginx/src/http/modules/ngx_http_stub_status_module.c)

根据修改可以变为如下代码
```c
//
// Created by helintong on 2/4/21.
//
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *ngx_http_hello(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_hello_handler(ngx_http_request_t *r);
// say_hello是写在配置文件中的
static ngx_command_t  ngx_http_hello_commands[] = {

        { ngx_string("say_hello"),
          NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS, // 让其允许配置到locatino中,没有参数
          ngx_http_hello,
          0,
          0,
          NULL },

        ngx_null_command
};


static ngx_http_module_t  ngx_http_hello_module_ctx = {
        NULL,                                   /* preconfiguration */
        NULL,                                  /* postconfiguration */

        NULL,                                  /* create main configuration */
        NULL,                                  /* init main configuration */

        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */

        NULL,                                  /* create location configuration */
        NULL                                   /* merge location configuration */
};

ngx_module_t  ngx_http_hello_module = {
        NGX_MODULE_V1,
        &ngx_http_hello_module_ctx,      /* module context */
        ngx_http_hello_commands,              /* module directives */
        NGX_HTTP_MODULE,                       /* module type */
        NULL,                                  /* init master */
        NULL,                                  /* init module */
        NULL,                                  /* init process */
        NULL,                                  /* init thread */
        NULL,                                  /* exit thread */
        NULL,                                  /* exit process */
        NULL,                                  /* exit master */
        NGX_MODULE_V1_PADDING
};

static char *
ngx_http_hello(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_hello_handler;

    return NGX_CONF_OK;
}
// 用来处理http请求的函数
static ngx_int_t ngx_http_hello_handler(ngx_http_request_t *r)
{
    ngx_chain_t        out;
    
    return ngx_http_output_filter(r, &out);
}
```

```nginx.conf
http{
    server {
    ...
        location / {
            say_hello;
        }
    }
    ...
}
```