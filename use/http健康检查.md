## http健康检查

最近看了APISIX（主要功能是API网关）开源项目，还有前不久看了斗鱼的open api的动态路由，于是总结下nginx对于backend server做健康检查，成文如下：

* [概述](#1)
* [被动检查](#2)
* [主动检查](#3)
* [nginx_upstream_check_module](#4)
* [lua-resty-upstream-healthcheck](#5)

<h3 id="1">概述</h3>
当我们通过nginx中转请求到backend server时，我们当然期望能有一种机制，就是探测backend server是否还能够服务，如果不能则从upstream servers里删除，以及当恢复服务时重新加入upstream servers，这中机制就是健康检查。

<h3 id="2">被动检查</h3>
Nginx 在代理请求过程中会自动的监测每个后端服务器对请求的响应状态，如果某个后端服务器对请求的响应状态在短时间内累计一定失败次数时，Nginx 将会标记该服务器异常。就不会转发流量给该服务器。 不过每间隔一段时间 Nginx 还是会转发少量的一些请求给该后端服务器来探测它的返回状态。 以便识别该服务器是否恢复。

``` javascript
upstream backend {
    server backend1.example.com;
    server backend2.example.com max_fails=3 fail_timeout=30s;
}
```

后端服务器不需要专门提供健康检查接口，不过这种方式会造成一些用户请求的响应失败，因为 Nginx 需要用一些少量的请求去试探后端的服务是否恢复正常。

注：如果是采用 Nginx 被动检查模式，官方原生的 Nginx 就支持，不需要依赖第三方模块或技术，所以下面的探讨都是针对 Nginx 实现主动健康检查的方法。

<h3 id="3">主动检查</h3>
Nginx 服务端会按照设定的间隔时间主动向后端的 upstream_server 发出检查请求来验证后端的各个 upstream_server 的状态。 如果得到某个服务器失败的返回超过一定次数，比如 3 次就会标记该服务器为异常，就不会将请求转发至该服务器。

``` javascript
upstream backend {
    server backend1.example.com；
    server backend2.example.com;
    server 192.0.0.1 backup;
}

server {
    location / {
        proxy_pass http://backend;
        health_check;
    }
}
```

如上配置，nginx默认会每隔5秒向每一个backend server发出 "/" 的请求，如果出现了超时或者无法建立连接则视为该backend server不可用，不会中转请求给该服务器，指导探测其又可用了，则重新加入到upstream server里。

当然我们可以指定某个端口去做健康检查：
``` javascript
upstream backend {
    server backend1.example.com；
    server backend2.example.com;
    server 192.0.0.1 backup;
}

server {
    location / {
        proxy_pass http://backend;
        health_check port=8080;
    }
}
```

另外我们还可以这样配置：

``` javascript
upstream backend {
    server backend1.example.com；
    server backend2.example.com;
    server 192.0.0.1 backup;
}

server {
    location / {
        proxy_pass http://backend;
        health_check interval=10 fails=3 passes=2 uri=/some/path port=8080;
    }
}
```
* interval : 指定健康检查是每隔10s去发送一次请求；
* fails ： 指定如果连续失败3次则认为backend server不可用，默认是一次；
* passes ：如果有两轮健康检查通过了，才能重新加入upstream servers；
* uri ：指定健康检查时请求的uri。


**这种主动检查只能在商用版的Nginx plus上使用，那么我们怎么办呢？**


<h3 id="4">nginx_upstream_check_module</h3>

对backend servers进行健康检查一个开源模块，至于安装方式可以参考[https://github.com/yaoweibin/nginx_upstream_check_module](https://github.com/yaoweibin/nginx_upstream_check_module)， 配置方式如下：

``` javascript
http {

        upstream cluster {

            # simple round-robin
            server 192.168.0.1:80;
            server 192.168.0.2:80;

            check interval=5000 rise=1 fall=3 timeout=4000;

            #check interval=3000 rise=2 fall=5 timeout=1000 type=ssl_hello;

            #check interval=3000 rise=2 fall=5 timeout=1000 type=http;
            #check_http_send "HEAD / HTTP/1.0\r\n\r\n";
            #check_http_expect_alive http_2xx http_3xx;
        }

        server {
            listen 80;

            location / {
                proxy_pass http://cluster;
            }

            location /status {
                check_status;

                access_log   off;
                allow SOME.IP.ADD.RESS;
                deny all;
           }
        }

    }
```
上面 配置的意思是，对name这个负载均衡条目中的所有节点，每隔5秒检测一次，请求1次正常则标记 upstream server状态为up，如果检测 3次都失败，则标记 realserver的状态为down，超时时间为4秒。

语法如下：

```
syntax: *check interval=milliseconds [fall=count] [rise=count]
    [timeout=milliseconds] [default_down=true|false]
    [type=tcp|http|ssl_hello|mysql|ajp|fastcgi]*

    default: *none, if parameters omitted, default parameters are
    interval=30000 fall=5 rise=2 timeout=1000 default_down=true type=tcp*
```

* interval：检测间隔时间，单位为毫秒。

* rsie：请求2次正常的话，标记此realserver的状态为up。

* fall：表示请求5次都失败的情况下，标记此realserver的状态为down。

* timeout：为超时时间，单位为毫秒。

* default_down: 设置backend server的初始状态 默认为down。

* port：健康检查时的端口，默认为0，表示使用backend server的原始处理请求的端口。

* type：健康检查时使用的协议 :
        1.  *tcp* is a simple tcp socket connect and peek one byte.  
        2.  *ssl_hello* sends a client ssl hello packet and receives the
            server ssl hello packet.  
        3.  *http* sends a http request packet, receives and parses the http
            response to diagnose if the upstream server is alive.  
        4.  *mysql* connects to the mysql server, receives the greeting
            response to diagnose if the upstream server is alive.  
        5.  *ajp* sends a AJP Cping packet, receives and parses the AJP
            Cpong response to diagnose if the upstream server is alive.  
        6.  *fastcgi* send a fastcgi request, receives and parses the
            fastcgi response to diagnose if the upstream server is alive.  

当然我们可以在server段里面可以加入查看realserver状态的页面

```
http {
        upstream cluster {

            # simple round-robin
            server 192.168.0.1:80;
            server 192.168.0.2:80;

            check interval=5000 rise=1 fall=3 timeout=4000;

            #check interval=3000 rise=2 fall=5 timeout=1000 type=ssl_hello;

            #check interval=3000 rise=2 fall=5 timeout=1000 type=http;
            #check_http_send "HEAD / HTTP/1.0\r\n\r\n";
            #check_http_expect_alive http_2xx http_3xx;
        }

        server {
            listen 80;

            location / {
                proxy_pass http://cluster;
            }

            location /status {
                check_status;

                access_log   off;
                allow SOME.IP.ADD.RESS;
                deny all;
           }
		   
		   location /nstatus {
               check_status;
               access_log off;
              #allow SOME.IP.ADD.RESS;
              #deny all;
           }
        }
    }
```

![upstream server的状态](./nstatus.jpg)

<h3 id="5">lua-resty-upstream-healthcheck</h3>
在nginx里我们可以使用nginx_upstream_check_module模块去进行健康检查。但是如果我们使用openresty（基于nginx + luaJIT封装的，提供了大量优良的lua库）的话，就多一种选择。

基本原理是在init_worker_by_lua_block 阶段，require “resty.upstream.healthcheck”，然后调用它的spawn_checker函数去做健康检查。至于如何使用它可以参考官方[https://github.com/openresty/lua-resty-upstream-healthcheck](https://github.com/openresty/lua-resty-upstream-healthcheck)的说明。

在apisix和kong开源项目中还有更高级的玩法，以后再撰文详述吧！