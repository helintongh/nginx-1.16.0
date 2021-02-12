//
// Created by helintong on 2/4/21.
//
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *ngx_http_hello(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_hello_handler(ngx_http_request_t *r);

static ngx_command_t  ngx_http_hello_commands[] = {

        { ngx_string("say_hello"),
          NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
          ngx_http_hello,
          0,
          0,
          NULL },

        ngx_null_command
};


static ngx_http_module_t  ngx_http_hello_module_ctx = {
        NULL,    /* preconfiguration */
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

static ngx_int_t ngx_http_hello_handler(ngx_http_request_t *r)
{

    ngx_int_t rc = ngx_http_discard_request_body(r);
    if(rc != NGX_OK)
    {
        return rc;
    }

    ngx_str_t content;
    /* 1.获取http请求的是什么方法 */
    // 这里有一个坑点,提交表单的情况nginx转换过后只有GET和POST请求
    switch(r->method)
    {
        case NGX_HTTP_PUT:
            ngx_str_set(&content, "PUT");
            break;
        case NGX_HTTP_GET:
            ngx_str_set(&content, "GET");
            break;
        case NGX_HTTP_POST:
            ngx_str_set(&content, "POST");
            break;
        default:
            ngx_str_set(&content, "OTHERS");
    }
    /* part 1 end */

    /* 注:与uri有关的需要把前端的POST方法改成GET方法。
            uri参数GET获取的仅需header。
            POST则把数据放到body里面取不到。
    */
    /* 2. 获取请求的uri,
        注:我使用的是同一变量content要看效果要注释非2 part的代码,后面的part亦然 */
    // contenx = r->uri; // 该语句只能获取到最基础的uri没法获取uri参数
    // content.data = r->uri_start;
    // content.len = r->uri_end - r->uri_start;
    /* part 2 end */

    /* 3. 获取uri参数 */
    // content = r->args; // 获取所有参数
    content.data = r->args_start;
    content.len = r->uri_end - r->args_start;
    /* part 3 end */



    ngx_str_t type = ngx_string("text/plain");
    //ngx_str_t content = ngx_string("hello world!");
    r->headers_out.content_type = type;
    r->headers_out.content_length_n = content;
    r->headers_out.status = NGX_HTTP_OK;
    rc = ngx_http_send_header(r); // 该函数发送http头(r的内容)给客户端
    if(rc != NGX_OK)
    {
        return rc;
    }

    // ngx_create_temp_buf相当于malloc,第一个参数是指定内存池,第二个参数是内容大小
    ngx_buf_t *b = ngx_create_temp_buf(r->pool, content.len);
    if(NULL == b)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    // 复制内容到b结点里,参数为b起始位置,content内容,数据长度
    ngx_memcpy(b->pos, content.data, content.len);
    b->last = b->pos + content.len; // 指定结点b末尾
    b->last_buf = 1; // 指明只有这一块buf没有分块

    ngx_chain_t        out;
    out.buf = b; //
    out.next = NULL; // 该示例只有一个结点
    
    return ngx_http_output_filter(r, &out);
}