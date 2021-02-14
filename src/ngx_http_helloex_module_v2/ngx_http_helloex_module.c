#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

//part 1
static ngx_int_t ngx_http_helloex_handler(ngx_http_request_t *r);
static void *ngx_http_helloex_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_helloex_init(ngx_conf_t *cf);

//part 2
typedef struct {
    ngx_str_t hello_str;
}ngx_http_helloex_conf_t;

//part 3
static ngx_command_t  ngx_http_helloex_commands[] = {

        { ngx_string("say_helloex"),
          NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,   
          ngx_conf_set_str_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_helloex_conf_t, hello_str),
          NULL },
        ngx_null_command
};

//part 4
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

//part 5
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

//part 6
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

    *h = ngx_http_helloex_handler;

    return NGX_OK;

}

static ngx_int_t ngx_http_helloex_handler(ngx_http_request_t *r)
{
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    //part 6
    ngx_http_helloex_conf_t  *nhhc;

    nhhc = ngx_http_get_module_loc_conf(r, ngx_http_helloex_module);
    // 申请一块内存
    ngx_buf_t       *pBuf = NULL;
    size_t          bufLen = 0;
    bufLen = sizeof("hello ")-1 + nhhc->hello_str.len;// 申请多少内存计算
    pBuf = ngx_create_temp_buf(r->pool, bufLen);
    if (pBuf == NULL) {
      return NGX_ERROR;
    }
    // 字符串拼接
    pBuf->last = ngx_sprintf(pBuf->last, "hello %V", &(nhhc->hello_str));
    pBuf->last_buf = 1; // 指出当前有几个buf
    ngx_str_t type = ngx_string("text/plain");
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = bufLen; // http头的content length
    r->headers_out.content_type = type;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    ngx_chain_t out;
    out.buf = pBuf; // 把pBuf结点挂到输出out上
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}