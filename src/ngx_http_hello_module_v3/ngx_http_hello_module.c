//
// Created by helintong on 2/4/21.
//
/*
request method
request URI/URL
request protocol
request header_in 原始请求头 headers_in 格式化后的请求头
request body 请求的body 
*/
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

	ngx_int_t rc = NGX_OK;
    // ngx_int_t rc = ngx_http_discard_request_body(r); // 不丢弃body体
    if(rc != NGX_OK)
    {
        return rc;
    }

    ngx_str_t content;

    /* 1. 获取http协议版本号,看效果去掉注释,后面亦然 */
    // content = r->http_protocol;
    // 下面才是常用的方法
    /*
	switch(r->http_version)
	{
		case NGX_HTTP_VERSION_11:
			break;
		case NGX_HTTP_VERSION_9:
			break
	}
    */
    /* end of part 1 */

    /* 2. http原始请求头获取*/
    // content.data = r->header_in->start;
    // content.len = r->header_in->end - r->header_in->start;
    /*
     part 2 代码会让你能够通过浏览器下载原始请求头信息多用于排查问题
     end of part 2
     */
    /* 3. 获取格式化后的http头信息 */
    // content = r->headers_in.host->value; // 获取host的值存到content中
    /* end of part 3*/

    /* 4. 获取body体的内容,注此时要把html中的login.html方法改为POST*/
    content.data = r->connection->buffer->pos;
    content.len = r->connection->buffer->last - r->connection->buffer->pos;
    /* end of part 4*/

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