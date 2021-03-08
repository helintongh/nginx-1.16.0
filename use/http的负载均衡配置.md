## http的负载均衡配置

* [概述](#1)
* [一个负载均衡的栗子](#2)
* [负载均衡策略](#3)
* [服务权重](#4)
* [session一致性](#5)
* [慢启动](#6)
* [连接数限制](#7)
* [健康检查](#8)
* [worker进程间共享数据](#9)
* [使用DNS做负载均衡](#10)
* [使用nginx API动态调整负载均衡配置](#11)


<h3 id="1">概述</h3>
负载均衡一般被用来优化资源利用率、最大化吞吐量、降低延迟和容错配置。

Nginx 可以作为一种十分有效的 HTTP 负载均衡工具来使用，通过 nginx 的负载均衡分发流量到不同的应用服务器，可以提升 web 应用的性能、伸展性和可靠性。


<h3 id="2">一个负载均衡的栗子</h3>
最简单的 nginx 负载均衡配置看起来像这样：

```
http {
    upstream myapp1 {
        server srv1.example.com;
        server srv2.example.com;
        server srv3.example.com;
    }

    server {
        listen 80;

        location / {
            proxy_pass http://myapp1;
        }
    }
}
```
在上面的例子中，有三个一样的应用跑在 srv1-srv3 服务器上。当我们没有配置任何负载均衡策略的时候，nginx 会采用默认的负载均衡策略——轮询。所有的请求都被代理到 myapp1 服务器群，然后 nginx 根据 HTTP 负载均衡分发请求。

Nginx 反向代理包括对 HTTP, HTTPS, FastCGI, uwsgi, SCGI, and memcached 的负载均衡策略。

如果想要配置 HTTPS 的负载均衡，只需要使用 “https” 协议就可以了。

当设置 FastCGI, uwsgi, SCGI, or memcached 的负载均衡策略时，只需要使用分别使用 fastcgi_pass , uwsgi_pass , scgi_pass , and memcached_pass 指令就可以了。

<h3 id="3">负载均衡策略</h3>
Nginx 支持下面几种负载均衡策略：

* round-robin(轮询) : 根据轮询分发请求到不同的服务器，轮询时也会考虑server的权重配置（见下文的server weight部分），这个是默认的配置。

```
upstream backend {
   # no load balancing method is specified for Round Robin
   server backend1.example.com;
   server backend2.example.com;
}
```

* least-connected(最少连接) : 将最新请求分发到活动连接最少的服务器，会考虑server weight的配比。

```
upstream backend {
    least_conn;
    server backend1.example.com;
    server backend2.example.com;
}
```

* ip-hash(ip 哈希) : 用一个哈希函数来决定最新请求应该被分发到哪一个服务器(基于客户端的 ip)，hash策略能够保证同一个客户端的请求会中转到同一个服务器去处理。

```
upstream backend {
    ip_hash;
    server backend1.example.com;
    server backend2.example.com;
}
```
在这个配置策略下，我们可以从负载均衡配置下临时去掉某个服务器，只需要把这个服务器标志为down就可以了，如下所示，当然这个时候，对客户端的请求也会依照IP做hash运算，并且把这个请求放在下一个服务器处理。

```
upstream backend {
    server backend1.example.com;
    server backend2.example.com;
    server backend3.example.com down;
}
```
* Generic Hash : 依据某个变量或者字符串去做hash运算，以此决定将用户请求中转到其对应的服务上处理，比如使用uri、ip或者其它，甚至是可以组合多个变量。举例如下：

```
upstream backend {
    hash $request_uri consistent;
    server backend1.example.com;
    server backend2.example.com;
}
```
consistent是一个可选的参数，hash运算时会使用ketama 一致性哈希去做负载均衡。引用原文说明下：
The optional consistent parameter to the hash directive enables ketama consistent‑hash load balancing. Requests are evenly distributed across all upstream servers based on the user‑defined hashed key value. If an upstream server is added to or removed from an upstream group, only a few keys are remapped which minimizes cache misses in the case of load‑balancing cache servers or other applications that accumulate state.

* Least Time ：依据最短平均延迟和最小连接数去决策，最短平均延时包含以下三种模式：
	* `header`: 从服务器接收到第一个字节的时间
	* `last_byte`: 从服务器接收到完整的 response 的时间
	* `last_byte inflight`: 从服务器接收到完整地 response 的时间（考虑不完整的请求）

* Random (随机算法)
顾名思义，每个请求都被随机发送到某个服务实例上去:
```
upstream backend {
    random;
    server backend1.example.com;
    server backend2.example.com;
    server backend3.example.com;
    server backend4.example.com;
}
```

Random 模式还提供了一个参数 two，当这个参数被指定时，NGINX 会先随机地选择两个服务器(考虑 weight)，然后用以下几种方法选择其中的一个服务器:
1. `least_conn`: 最少连接数
2. `least_time=header(NGINX PLUS only)`: 接收到 response header 的最短平均时间
3. `least_time=last_byte(NGINX PLUS only)`: 接收到完整请求的最短平均时间

我们可以参考下面的一个例子:
```
upstream backend {
    random two least_time=last_byte;
    server backend1.example.com;
    server backend2.example.com;
    server backend3.example.com;
    server backend4.example.com;
}
```
当环境中有多个负载均衡服务器在向后端服务转发请求时，我们可以考虑使用 Random 模式，在只有单个负载均衡服务器时，一般不建议使用 Random 模式。

<h3 id="4">服务权重</h3>
除了上面几种种负载均衡策略，我们还可以通过配置服务器权重来更深入地影响 nginx 负载均衡算法。

在上面几个例子中，并没有配置服务器权重，那么这意味着 nginx 在进行负载均衡计算的时候会同等地看待配置的服务器。

假如有足够的请求，并且请求的处理模式一致而且完成的速度足够快，那么轮询负载均衡策略意味着根据基本上一致的权重来分发请求到服务器上。

当一个服务器被配置了权重的时候，权重值就会被当做负载均衡算法决策因素的一部分。
```
upstream myapp1 {
    server srv1.example.com weight=3;
    server srv2.example.com;
    server srv3.example.com;
	server srv4.example.com backup;
}
```
当采用上面的配置的时候，每5个请求将会如下方式分发到应用服务器上：
三个请求会分发到 srv1，一个会分发到 srv2，另一个会分发到 srv3

权重式负载均衡策略可以在最近版本的 nginx 运用到最少连接和 ip hash 负载均衡策略中。

<h3 id="5">session一致性</h3>

开启Session保持, 即nginx将标识用户session, 并且将该session的request都转发到相同的Web服务器, nginx支持3种会话保持策略(需要nginx-sticky-module模块):

1. sticky cookie, 当web服务器第一次响应request时, nginx会向client写入一个cookie, 下次client再请求, 则会被转发到同一台机器:

```
upstream web_server {
    server 192.168.1.14:8080; # web1
    server 192.168.1.10:8080; # web2
    sticky cookie {cookie_name} expires=1h domain={cookie域} path={cookie路径};
}
```

2. sticky route, 当nginx收到client第一个请求时, 会分配一个叫"route"的cookie给client, 后续请求将被转发到对应route的Web服务器上:

```
map $cookie_jsessionid $route_cookie {
    ~.+\.(?P\w+)$ $route;
}

map $request_uri $route_uri {
    ~jsessionid=.+\.(?P\w+)$ $route;
}

upstream web_server {
    server 192.168.1.14:8080 route=web1; # web1
    server 192.168.1.10:8080 route=web2; # web2
    sticky route $route_cookie $route_uri;
}
```

3. cookie learn, nginx第一次会根据request和response找到session标识符, 并且nginx会学会哪个web服务器对应哪个session标识符, 这些标识符通常在HTTP Cookie中:
```
upstream web_server {
    server 192.168.1.14:8080; # web1
    server 192.168.1.10:8080; # web2

    sticky learn
        create=$upstream_cookie_sessionid # 必须, nginx会通过cookie中的SESSIONID来创建会话
        lookup=$cookie_sessionid          # 必须, 通过cookie中的SESSIONID来查询存在的会话
        zone=client_sessions:1m           # 必须, 指定session信息保存的内存区域叫client_seesions, 并1M大小
        timeout=1h;
}
```

<h3 id="6">慢启动</h3>
如果一个服务器刚重启或者刚刚变成可用状态，此时TCP连接会处于一个慢启动的过程，经过一段时间后才能满负荷传输，因此如果在慢启动过程中，如果我们大量的发送请求，则有可能导致请求超时，且被nginx标记为failed状态。

nginx支持慢启动的特性，允许服务器处理的权重从0恢复到配置的权重，通过slow_start参数去配置，如下：

```
upstream backend {
    server backend1.example.com slow_start=30s;
    server backend2.example.com;
    server 192.0.0.1 backup;
}
```
slow_start是一个时间值，意味需要30s才能恢复到权重值。


* [连接数限制](#7)
通过max_conns参数可以设置upstream server的最大连接数量，这个达到这么多个连接数量了，那么会把用户请求放入一个队列里，当然我们可以使用queue参数去定义队列的大小。配置如下：

```
upstream backend {
    server backend1.example.com max_conns=3;
    server backend2.example.com;
    queue 100 timeout=70;
}
```

如果queue填充满了，或者这个upstream server在time out时间内不可用，则nginx会返回502错误给用户（If the queue is filled up with requests or the upstream server cannot be selected during the timeout specified by the optional timeout parameter, the client receives an error.）。

Note that the max_conns limit is ignored if there are idle [keepalive](https://nginx.org/en/docs/http/ngx_http_upstream_module.html?&_ga=2.98283034.44636628.1564490140-1323322656.1558925809#keepalive) connections opened in other worker processes. As a result, the total number of connections to the server might exceed the max_conns value in a configuration where the memory is shared with multiple worker processes.

<h3 id="8">健康检查</h3>
nginx可以持续地检查upstream的服务器的健康状态，避免中转一些请求给不可用的服务器。具体可以参考 [健康检查](https://docs.nginx.com/nginx/admin-guide/load-balancer/http-health-check/)的说明，或者是 [backend servers 健康检查](http://106.52.144.219:8888/0voice/nginx-docs/blob/master/nginx%E4%BB%8B%E7%BB%8D/http%E5%81%A5%E5%BA%B7%E6%A3%80%E6%9F%A5.md)。

<h3 id="9">worker进程间共享数据</h3>
如果上游块不包含zone指令，则每个工作进程都会保留其自己的服务器组配置副本，并维护自己的一组相关计数器。 计数器包括组中每个服务器的当前连接数以及将请求传递到服务器的失败尝试次数。 因此，无法动态修改服务器组配置。

当zone指令包含在上游块中时，上游组的配置保存在所有工作进程之间共享的内存区域中。 此方案是可动态配置的，因为工作进程访问组配置的相同副本并使用相同的相关计数器。

zone指令对于活动运行状况检查和上游组的动态重新配置是必需的。 但是，上游组的其他功能也可以从该指令的使用中受益。

例如，如果未共享组的配置，则每个工作进程都会维护自己的计数器，以便将请求传递给服务器（由max_fails参数设置失败）。 在这种情况下，每个请求只能访问一个工作进程。 当选择处理请求的工作进程无法将请求传输到服务器时，其他工作进程对此一无所知。 虽然某些工作进程可以认为服务器不可用，但其他人可能仍会向此服务器发送请求。 对于最终被视为不可用的服务器，fail_timeout参数设置的时间范围内的失败尝试次数必须等于max_fails乘以工作进程数。 另一方面，zone指令保证了预期的行为。

同样，如果没有zone指令，Least Connections负载平衡方法可能无法正常工作，至少在低负载下。 此方法将请求传递给具有最少活动连接数的服务器。 如果未共享组的配置，则每个工作进程使用其自己的计数器作为连接数，并可能将请求发送到另一个工作进程刚刚发送请求的同一服务器。 但是，您可以增加请求数以减少此影响。 在高负载下，请求在工作进程之间均匀分布，并且最少连接方法按预期工作。

#### 设置zone大小
不可能推荐理想的内存区大小，因为使用模式差异很大。 所需的内存量取决于启用了哪些功能（例如会话一致性，运行状况检查或DNS重新解析）以及如何识别上游服务器。

例如，使用sticky_route会话一致性策略并启用单个运行状况检查，256 KB区域可以容纳有关指示的上游服务器数量的信息：

128台服务器（每台服务器定义为IP地址：端口对）
88个服务器（每个服务器定义为主机名：主机名解析为单个IP地址的端口对）
12个服务器（每个服务器定义为主机名：主机名解析为多个IP地址的端口对）


<h3 id="10">使用DNS做负载均衡</h3>
Nginx的backend group中的主机可以配置成域名的形式，如果在域名的后面添加resolve参数，那么Nginx会周期性的解析这个域名，当域名解析的结果发生变化的时候会自动生效而不用重启。

```
http {
    resolver 10.0.0.1 valid=300s ipv6=off;
    resolver_timeout 10s;
    server {
        location / {
            proxy_pass http://backend;
        }
    }
   
    upstream backend {
        zone backend 32k;
        least_conn;
        ...
        server backend1.example.com resolve;
        server backend2.example.com resolve;
    }
}
```

　　如果域名解析的结果含有多个IP地址，这些IP地址都会保存到配置文件中去，并且这些IP都参与到自动负载均衡。

<h3 id="11">使用nginx API动态调整负载均衡配置</h3>

通过Nginx扩展, 上游服务器组地配置可以通过Nginx扩展API进行动态修改. 配置命令可以作用组内地所有或者部分服务器, 修改部分服务器的参数, 增加或删除服务器，见后续文章。需要引入模块[ngx_http_upstream_conf_module](http://nginx.org/en/docs/http/ngx_http_upstream_conf_module.html)，但是这个是Nginx Plus 的，商用的，现在有很多其它的模块来实现的。比如openresty的balancer_by_lua，又比如[ngx_dynamic_upstream](https://github.com/cubicdaiya/ngx_dynamic_upstream)，当然还有很多其它公司开发的。

参考文档：https://nginx.org/en/docs/http/ngx_http_upstream_module.html?&_ga=2.97830170.44636628.1564490140-1323322656.1558925809