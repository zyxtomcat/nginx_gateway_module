/*
 * Copyright (c) 2013 Zhuyx
 */

 #include <ngx_config.h>
 #include <ngx_core.h>
 #include <ngx_event.h>
 #include <ngx_gateway.h>
 #include <nginx.h>


 static void *ngx_gateway_core_create_main_conf(ngx_conf_t *cf);
 static void *ngx_gateway_core_create_srv_conf(ngx_conf_t *cf);
 static void *ngx_gateway_core_create_biz_conf(ngx_conf_t *cf);
 static char *ngx_gateway_core_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
 static char *ngx_gateway_core_merge_biz_conf(ngx_conf_t *cf, void *parnet, void *child);

 static char *ngx_gateway_core_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
 static char *ngx_gateway_core_listen(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
 static char *ngx_gateway_core_business(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
 static char *ngx_gateway_core_protocol(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
 static char *ngx_gateway_core_resolver(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
 static char *ngx_gateway_core_access_rule(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
 static char *ngx_gateway_log_set_access_rule(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

 static ngx_command_t ngx_gateway_core_commands[] = {

 	{
 		ngx_string("server"),
 		NGX_GATEWAY_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_MULTI|NGX_CONF_NOARGS.
 		ngx_gateway_core_server,
 		0,
 		0,
 		NULL
 	},

 	{
 		ngx_string("listen"),
 		NGX_GATEWAY_SRV_CONF|NGX_CONF_1MORE,
 		ngx_gateway_core_listen,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		0,
 		NULL
 	},

 	{
 		ngx_string("business"),
 		NGX_GATEWAY_SRV_CONF|NGX_GATEWAY_BIZ_CONF|NGX_CONF_BLOCK|NGX_CONF_TAKE1,
 		ngx_gateway_core_business,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		0,
 		NULL
 	},

 	{
 		ngx_string("protocol"),
 		NGX_GATEWAY_SRV_CONF|NGX_CONF_TAKE1,
 		ngx_gateway_core_protocol,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		0,
 		NULL
 	},

 	{
 		ngx_string("so_keepalive"),
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_FLAG,
 		ngx_conf_set_flag_slot,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		offsetof(ngx_gateway_core_srv_conf_t,so_keepalive),
 		NULL
 	},

 	{
 		ngx_string("tcp_nodelay"),
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_FLAG,
 		ngx_conf_set_flag_slot,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		offsetof(ngx_gateway_core_srv_conf_t, tcp_nodelay),
 		NULL
 	},

 	{
 		ngx_string("timeout"),
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_TAKE1,
 		ngx_conf_set_msec_slot,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		offsetof(ngx_gateway_core_srv_conf_t, timeout),
 		NULL
 	},

 	{
 		ngx_string("resolver"),
 #if defined(nginx_version) && (nginx_version >= 1001007)
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_1MORE,
 #else
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_TAKE1,
 #endif
 		ngx_gateway_core_resolver,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		0,
 		NULL
 	},

 	{
 		ngx_string("resolver_timeout"),
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_TAKE1,
 		ngx_conf_set_msec_slot,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		0,
 		NULL
 	},

 	{
 		ngx_string("allow"),
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_TAKE1,
 		ngx_gateway_core_access_rule,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		0,
 		NULL
 	},

 	{
 		ngx_string("deny"),
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_TAKE1,
 		ngx_gateway_core_access_rule,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		0,
 		NULL
 	},

 	{
 		ngx_string("access_log"),
 		NGX_GATEWAY_MAIN_CONF|NGX_GATEWAY_SRV_CONF|NGX_CONF_TAKE12,
 		ngx_gateway_log_set_access_rule,
 		NGX_GATEWAY_SRV_CONF_OFFSET,
 		0,
 		NULL
 	},

 	ngx_null_command

 };

 static ngx_gateway_module_t ngx_gateway_core_module_ctx = {
 	NULL,											/* protocol */

 	ngx_gateway_core_create_main_conf,				/* create main configuration */
 	NULL,											/* init main configuration */

 	ngx_gateway_core_create_srv_conf,				/* create srv configuration */
 	ngx_gateway_core_merge_srv_conf,				/* merge srv configuration */

 	ngx_gateway_core_create_biz_conf,				/* create biz configuration */
 	ngx_gateway_core_merge_biz_conf					/* merge biz configuration*/
 };

 ngx_module_t ngx_gateway_core_module = {
 	NGX_MODULE_V1,
 	&ngx_gateway_core_module_ctx,					/* module context */
 	ngx_gateway_core_commands,						/* module directives */
 	NGX_GATEWAY_MODULE,								/* module type */
 	NULL,											/* init master */
 	NULL,											/* init module */
 	NULL,											/* init process */
 	NULL,											/* init thread */
 	NULL,											/* exit thread */
 	NULL,											/* exit process */
 	NULL,											/* exit master */
 	NGX_MODULE_VI_PADDING
 };

 static void *
 ngx_gateway_core_create_main_conf(ngx_conf_t *cf)
 {
 	ngx_gateway_core_main_conf_t *cmcf;

 	cmcf = ngx_pcalloc(cf->pool, sizeof(ngx_gateway_core_main_conf_t));
 	if (NULL == cmcf)
 	{
 		return NULL;
 	}

 	if (ngx_array_init(&cmcf->servers, cf->pool, 4,
 						sizeof(ngx_gateway_core_srv_conf_t *))
 		!= NGX_OK)
 	{
 		return NULL;
 	}

 	if (ngx_array_init(&cmcf->listen, cf->pool, 4, sizeof(ngx_gateway_listen_t))
 		!= NGX_OK )
 	{
 		return NULL;
 	}

 	return cmcf;
 }

 static void *
 ngx_gateway_core_create_srv_conf(ngx_conf_t *cf)
 {
 	ngx_gateway_core_srv_conf_t *cscf;

 	cscf = ngx_pcalloc(cf->pool, sizeof(ngx_gateway_core_srv_conf_t));
 	if (NULL == cscf)
 	{
 		return NULL;
 	}

 	if (ngx_array_init(&cscf->business, cf->pool, 4, sizeof(ngx_gateway_core_biz_conf_t* ))
 		!= NGX_OK)
 	{
 		return NULL;
 	}

 	cscf->timeout = NGX_CONF_UNSET_MSEC;
 	cscf->resolver_timeout = NGX_CONF_UNSET_MSEC;
 	cscf->so_keepalive = NGX_CONF_UNSET;
 	cscf->tcp_nodelay = NGX_CONF_UNSET;

 	cscf->resolver = NGX_CONF_UNSET_PTR;

 	cscf->file_name = cf->conf_file->file.name.data;
 	cscf->line = cf->conf_file->line;

 	cscf->access_log = ngx_pcalloc(cf->pool, sizeof(ngx_gateway_log_srv_conf_t));
 	if (NULL == cscf->access_log)
 	{
 		return NULL;
 	}

 	cscf->access_log->open_file_cache = NGX_CONF_UNSET_PTR;

 	return cscf;
 }

 static char *
 ngx_gateway_core_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
 {
 	ngx_uint_t					m;
 	ngx_gateway_log_t 			*log;
 	ngx_gateway_module_t 		*module;
 	ngx_gateway_core_srv_conf_t	*prev = parent;
 	ngx_gateway_core_srv_conf_t *conf = child;
 	ngx_gateway_log_srv_conf_t	*plscf = prev->access_log;
 	ngx_gateway_log_srv_conf_t	*lscf = conf->access_log;

 	ngx_conf_merge_msec_value(conf->timeout, prev->timeout, 60000);
 	ngx_conf_merge_msec_value(conf->resolver_timeout, prev->resolver_timeout, 30000);

 	ngx_conf_merge_value(conf->so_keepalive, prev->so_keepalive, 0);
 	ngx_conf_merge_value(conf->tcp_nodelay, prev->tcp_nodelay, 1);

 	ngx_conf_merge_ptr_value(conf->resolver, prev->resolver, NULL);
 	ngx_conf_merge_ptr_value(conf->rules, prev->rules, NULL);

 	if ( NULL == conf->protocol )
 	{
 		for (m = 0; ngx_modules[m]; ++m) {
 			if (ngx_modules[m]->type != NGX_GATEWAY_MODULE) {
 				continue;
 			}

 			module = ngx_modules[m]->ctx;

 			if (module->protocol && ngx_strcmp(module->protocol->name.data, "tcp_proxy_generic") == 0 )
 			{
 				conf->protocol = module->protocol;
 			}
 		}
 	}

 	if (lscf->open_file_cache == NGX_CONF_UNSET_PTR)
 	{
 		lscf->open_file_cache = plscf->open_file_cache;
 		lscf->open_file_cache_valid = plscf->open_file_cache_valid;
 		lscf->open_file_cache_min_uses = plscf->open_file_cache_min_uses;

 		if (lscf->open_file_cache == NGX_CONF_UNSET_PTR)
 		{
 			lscf->open_file_cache = NULL;
 		}
 	}

 	if (lscf->logs || lscf->off)
 	{
 		return NGX_CONF_OK;
 	}

 	lscf->logs = plscf->logs;
 	lscf->off = plscf->off;

 	if (lscf->logs || lscf->off)
 	{
 		return NGX_CONF_OK;
 	}

 	lscf->logs = ngx_array_create(cf->pool, 2, sizeof(ngx_gateway_log_t));
 	if (NULL == lscf->logs) {
 		return NGX_CONF_ERROR;
 	}

 	log = ngx_array_push(lscf->logs);
 	if (NULL == log) {
 		return NGX_CONF_ERROR;
 	}

 	log->file = ngx_conf_open_file(cf->cycle, &ngx_gateway_access_log);
 	if (NULL == log->file) {
 		return NGX_CONF_ERROR;
 	}

 	log->disk_full_time = 0;
 	log->error_log_time = 0;

 	return NGX_CONG_OK;
 }

 static void *
 ngx_gateway_core_create_biz_conf(ngx_conf_t *cf)
 {
 	ngx_gateway_core_biz_conf_t *cbcf;

 	cbcf = ngx_pcalloc(cf->pool, sizeof(ngx_gateway_core_biz_conf_t));
 	if (NULL == cbcf) {
 		return NULL;
 	}

 	if (ngx_array_init(&cbcf->business, cf->pool, 4, sizeof(ngx_gateway_core_biz_conf_t *))
 		!= NGX_OK)
 	{
 		return NULL;
 	}

 	return cbcf;
 }

 static char *
 ngx_gateway_core_merge_biz_conf(ngx_conf_t *cf, void *parent, void *child)
 {
 	ngx_gateway_core_biz_conf_t *prev = parent;
 	ngx_gateway_core_biz_conf_t *conf = child;

 	(void)prev;
 	(void)conf;

 	return NGX_CONF_OK;
 }

 static char *
 ngx_gateway_core_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
 {
 	
 }