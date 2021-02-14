//
// Created by root on 20-1-1.
//

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static void *ngx_http_downloadex_create_loc_conf(ngx_conf_t *cf);
//static ngx_int_t ngx_http_downloadex_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_downloadex_handler(ngx_http_request_t *r);
static char *ngx_http_downloadex(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

typedef struct {
    ngx_str_t str_path;
}ngx_http_downloadex_conf_t;

static ngx_command_t ngx_http_downloadex_commands[]={
        {
          ngx_string("downloadex_file") ,
          NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
          ngx_http_downloadex,
          NGX_HTTP_LOC_CONF_OFFSET,
          0,
          NULL
        },
        ngx_null_command
};

static ngx_http_module_t  ngx_http_downloadex_module_ctx = {
        NULL,                                  /* preconfiguration */
        NULL,                        /* postconfiguration */
        NULL,                                  /* create main configuration */
        NULL,                                  /* init main configuration */
        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */
        ngx_http_downloadex_create_loc_conf,      /* create location configuration */
        NULL                                   /* merge location configuration */
};

ngx_module_t ngx_http_downloadex_module = {
        NGX_MODULE_V1,
        &ngx_http_downloadex_module_ctx,
        ngx_http_downloadex_commands,
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

static void *ngx_http_downloadex_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_downloadex_conf_t *mycf;
    mycf = (ngx_http_downloadex_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_downloadex_conf_t));
    if(mycf == NULL)
    {
        return NULL;
    }

    return mycf;
}

// static ngx_int_t ngx_http_downloadex_init(ngx_conf_t *cf)
// {
//     ngx_http_handler_pt        *h;
//     ngx_http_core_main_conf_t  *cmcf;

//     cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

//     h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
//     if (h == NULL) {
//         return NGX_ERROR;
//     }

//     *h = ngx_http_downloadex_handler;

//     return NGX_OK;
// }

static char *ngx_http_downloadex(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_str_t        *field, *value;
    ngx_conf_post_t  *post;

    field = (ngx_str_t *) (p + cmd->offset);

    if (field->data) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *field = value[1];

    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_downloadex_handler;

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, field);
    }
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_downloadex_handler(ngx_http_request_t *r)
{
    // 必须是GET或者HEAD方法，否则返回405 Not Allowed
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
        return NGX_HTTP_NOT_ALLOWED;
 
    // 丢弃请求中的包体
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK)
        return rc;
 
    ngx_buf_t *b;
    b = ngx_palloc(r->pool, sizeof(ngx_buf_t));
    // 取出配置中的参数值
    ngx_http_downloadex_conf_t  *nhdc;

    nhdc = ngx_http_get_module_loc_conf(r, ngx_http_downloadex_module);

    
   
    b->in_file = 1;     // 设置为1表示缓冲区中发送的是文件
 
    // 分配代表文件的结构体空间。file成员表示缓冲区引用的文件
    b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
    b->file->fd = ngx_open_file(nhdc->str_path.data, NGX_FILE_RDONLY | NGX_FILE_NONBLOCK, NGX_FILE_OPEN, 0);
    b->file->log = r->connection->log;  // 日志对象
    b->file->name.data = nhdc->str_path.data;      // name成员表示文件名称称
    b->file->name.len = nhdc->str_path.len;
    if (b->file->fd <= 0)
        return NGX_HTTP_NOT_FOUND;
 
    r->allow_ranges = 1;    //支持断点续传
 
    // 获取文件长度，ngx_file_info方法封装了stat系统调用
    // info成员就表示stat结构体
    if (ngx_file_info(nhdc->str_path.data, &b->file->info) == NGX_FILE_ERROR)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
 
    // 设置缓冲区指向的文件块
    b->file_pos = 0;                        // 文件起始位置
    b->file_last = b->file->info.st_size;   // 文件结束为止
 
    // 用于告诉HTTP框架。请求结束时调用cln->handler成员函数
    ngx_pool_cleanup_t* cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_pool_cleanup_file_t));
    if (cln == NULL)
        return NGX_ERROR;
 
    cln->handler = ngx_pool_cleanup_file;       // ngx_pool_cleanup_file专用于关闭文件句柄
     
    ngx_pool_cleanup_file_t  *clnf = cln->data; // cln->data为上述回调函数的參数
    clnf->fd = b->file->fd;
    clnf->name = b->file->name.data;
    clnf->log = r->pool->log;
 
    // 设置返回的Content-Type
    // 注意，ngx_str_t有一个非常方便的初始化宏
    // ngx_string，它能够把ngx_str_t的data和len成员都设置好
    ngx_str_t type = ngx_string("text/plain");
 
    //设置返回状态码
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = b->file->info.st_size;    // 正文长度
    r->headers_out.content_type = type;
 
    // 发送http头部
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
        return rc;
 
    // 构造发送时的ngx_chain_t结构体
    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;
 
    //最后一步发送包体，http框架会调用ngx_http_finalize_request方法
    return ngx_http_output_filter(r, &out);
}













