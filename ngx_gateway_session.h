/*
 * Copyright (c) 2013 zhuyx
 */

 #ifndef _NGX_GATEWAY_SESSION_H_INCLUDED_
 #define _NGX_GATEWAY_SESSION_H_INCLUDED_


 #include <ngx_config.h>
 #include <ngx_core.h>
 #include <ngx_event.h>
 #include <ngx_event_connect.h>
 #include <ngx_gateway.h>

 typedef struct ngx_gateway_session_s{
 	ngx_uint_t					signature;

 	ngx_pool_t					*pool;
 	ngx_connect_t				*connect;
 	ngx_gateway_upstream_t		*upstream;

 	ngx_str_t 					out;
 	ngx_buf_t					*buffer;

 	void						**ctx;
 	void						**main_conf;
 	void						**srv_conf;
 	void						**biz_conf;

 	ngx_resolver_ctx_t			*resolver_ctx;

 	time_t						start_sec;
 	ngx_msec_t					start_msec;

 	off_t						bytes_read;
 	off_t						bytes_write;


 } ngx_gateway_session_t;

 typedef void (*ngx_gateway_cleanup_pt)(void *data);

 struct ngx_gateway_clean_s {
 	ngx_gateway_cleanup_pt		handler;
 	void						*data;
 	ngx_gateway_cleanup_t 		*next;
 };

 #endif