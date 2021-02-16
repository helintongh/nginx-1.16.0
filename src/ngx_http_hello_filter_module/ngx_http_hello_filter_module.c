#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t ngx_http_hello_filter_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_hello_filter_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_hello_filter_body_filter(ngx_http_request_t *r, ngx_chain_t *in);
static char *ngx_http_hello_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
// 过滤模块链表入口
static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;
// 前缀定义
static ngx_str_t filter_prefix = ngx_string("[hello I am helintongh]");

static ngx_command_t ngx_http_hello_filter_commands[] = {
	{ ngx_string("hello_filter"), // 命令的名字
	  NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
	  ngx_http_hello_filter, // nginx需要执行该指针指向的函数
	  0,
	  0,
	  NULL },
	  ngx_null_command
};
// 上下文结构体
typedef struct {
	ngx_uint_t add_prefix;
}ngx_http_hello_filter_ctx_t;

static ngx_http_module_t  ngx_http_hello_filter_module_ctx = {
        NULL,    							   /* preconfiguration */
        ngx_http_hello_filter_init,            /* postconfiguration */
        NULL,                                  /* create main configuration */
        NULL,                                  /* init main configuration */
        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */
        NULL,                                  /* create location configuration */
        NULL                                   /* merge location configuration */
};

ngx_module_t  ngx_http_hello_filter_module = {
        NGX_MODULE_V1,
        &ngx_http_hello_filter_module_ctx,      /* module context */
        ngx_http_hello_filter_commands,              /* module directives */
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

static ngx_int_t ngx_http_hello_filter_init(ngx_conf_t *cf)
{
	// ngx_http_top_header_filter链表表头
	ngx_http_next_header_filter = ngx_http_top_header_filter;
	// ngx_http_hello_filter_header_filter函数占据表头并执行
	ngx_http_top_header_filter = ngx_http_hello_filter_header_filter;

	ngx_http_next_body_filter = ngx_http_top_body_filter;
	ngx_http_top_body_filter = ngx_htttp_hello_filter_body_filter;

	return NGX_OK;
}

static char *ngx_http_hello_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	return NGX_CONF_OK;
}
// 处理http协议头
/* 该函数做4个事情
1. 判断请求的状态是否为200,不是则不处理
2. 判断当前请求是否处理过,否则不处理
3. 申请上下文结构体内存,并赋初值
4. 判断文件类型是否符合要求,不符合要求不处理
5. 判断原本返回的响应有无内容,无则不处理
*/
static ngx_int_t ngx_http_hello_filter_header_filter(ngx_http_request_t *r)
{
	ngx_http_hello_filter_ctx_t *ctx;// 上下文变量
	// 用户请求状态码是否是200
	if(r->headers_out.status != NGX_HTTP_OK)
	{
		return ngx_http_next_header_filter(r); // 走到下一个过滤模块(不走自己写的过滤模块)
	}
	ctx = ngx_http_get_module_ctx(r, ngx_http_hello_filter_module);
	if(ctx) // 取出的不为空,已经处理过。走下一个过滤模块
	{
		return ngx_http_next_header_filter(r);
	}
	// 没有处理过,申请内存
	ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_hello_filter_module));
	if(ctx == NULL)
	{
		return NGX_ERROR;
	}
	// ctx的变量赋值
	ctx->add_prefix = 0;
	// 把ctx设置到ngx_http_hello_filter_module模块中
	ngx_http_set_ctx(r, ctx, ngx_http_hello_filter_module);
	// 比较当前的类型,如果是一个网页才进行处理
	if(r->headers_out.content_type.len >= sizeof("text/html") - 1 && 
		ngx_strncasecmp(r->headers_out.content_type.data, (u_char *)"text/html",sizeof("text/html") - 1) == 0)
	{
		ctx->add_prefix = 1;
		if(r->headers_out.content_length_n > 0) // 当前返回内容要大于0
		{
			r->headers_out.content_length_n += filter_prefix.len;
		}

	}
	return ngx_http_next_header_filter(r); 
}
// 处理http的body体
/*
1. 取出上下文,上下文不符合要求不处理
2. 标记上下文,避免重复操作
3. 申请内存,并存储前缀内容
4. 挂载到处理链表中并返回链表
*/
static ngx_int_t ngx_http_hello_filter_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	ngx_http_hello_filter_ctx_t *ctx; // 定义上下文
	ctx = ngx_http_get_module_ctx(r, ngx_http_hello_filter_module);// 取出上下文
	if(ctx == NULL || ctx->add_prefix != 1)// 是否为空或已经处理过
	{
		return ngx_http_next_body_filter(r, in); // 走下一条
	}
	ctx->add_prefix = 2;// 没处理过赋值为2
	ngx_buf_t *b = ngx_create_temp_buf(r->pool, filter_prefix.len);
	b->start = b->pos = filter_prefix.data;
	b->last = b->pos + filter_prefix.len;
	ngx_chain_t *cl = ngx_alloc_chain_link(r->pool);
	cl->buf = b;
	cl->next = in;
	return ngx_http_next_body_filter(r, cl);
}