#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
/*
配置块
http {
	server{
	...
	location /hello {
		say_helloex helintong; 
	}
	...
	}
}
上述访问/hello就会返回helintong
*/
//part 1 声明函数
static ngx_int_t ngx_http_helloex_handler(ngx_http_request_t *r); // 处理函数
static void *ngx_http_helloex_create_loc_conf(ngx_conf_t *cf); // 初始化参数变量
static ngx_int_t ngx_http_helloex_init(ngx_conf_t *cf); // 将处理函数挂在主流程

//part 2 存储用户配置参数的变量
typedef struct {
    ngx_str_t hello_str;
}ngx_http_helloex_conf_t;

//part 3 命令行数组
static ngx_command_t  ngx_http_helloex_commands[] = {

        { ngx_string("say_helloex"),
          NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,   // 代表放在http的location块,有一个参数
          ngx_conf_set_str_slot,	// 调用系统函数参数处理
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_helloex_conf_t, hello_str),
          NULL },
        ngx_null_command
};

//part 4, 设置相应的函数来处理配置变量(无参就全为NULL)
static ngx_http_module_t  ngx_http_helloex_module_ctx = {
        NULL,                                  /* preconfiguration */
        ngx_http_helloex_init,                 /* postconfiguration */
        NULL,                                  /* create main configuration */
        NULL,                                  /* init main configuration */
        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */
        ngx_http_helloex_create_loc_conf,      /* create location configuration */
        NULL                                   /* merge location configuration */
};



ngx_module_t ngx_http_helloex_module = {
        NGX_MODULE_V1,
        &ngx_http_helloex_module_ctx,
        ngx_http_helloex_commands,
        NGX_HTTP_MODULE,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NGX_MODULE_V1_PADDING
};

//part 5,初始化参数变量
static void *ngx_http_helloex_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_helloex_conf_t *mycf;
    mycf = (ngx_http_helloex_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_helloex_conf_t));
    if(mycf == NULL)
    {
        return NULL;
    }

    return mycf;
}

//part 6,将处理函数挂在主流程
static ngx_int_t
ngx_http_helloex_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_helloex_handler; // 处理函数变为ngx_http_helloex_handler

    return NGX_OK;

}
//part 6,处理函数
static ngx_int_t ngx_http_helloex_handler(ngx_http_request_t *r)
{
     ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    //part 7,获取配置项中的参数
    ngx_http_helloex_conf_t  *nhhc;
    // 通过nhhc指针接收nginx.conf写的参数值,第二个参数是第二个模块的module
    nhhc = ngx_http_get_module_loc_conf(r, ngx_http_helloex_module);
    ngx_str_t type = ngx_string("text/plain");
    ngx_str_t response = nhhc->hello_str; // 参数内容放入要返回给用户的变量response里
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = response.len;
    r->headers_out.content_type = type;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    ngx_buf_t *b;
    b = ngx_create_temp_buf(r->pool, response.len);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_memcpy(b->pos, response.data, response.len);
    b->last = b->pos + response.len;
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}