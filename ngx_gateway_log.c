/*
 * Copyright (c) 2013 Zhuyx
 */

 #include <ngx_config.h>
 #include <ngx_core.h>
 #include <ngx_gateway.h>

 static u_char * ngx_gateway_time(u_char *buf, time_t t);
 static u_char * ngx_gateway_log_fill(ngx_gateway_session_t *s, u_char *buf);
 static void ngx_gateway_log_write(ngx_gateway_session_t *s, ngx_gateway_log_t *log, u_char *buf, size_t len);

 ngx_int_t 
 ngx_gateway_log_handler(ngx_gateway_session_t *s)
 {
 	u_char							*line, *p;
 	size_t							len;
 	ngx_uint_t						l;
 	ngx_connection_t				*c;
 	ngx_gateway_log_t 				*log;
 	ngx_open_file_t 				*file;
 #if defined(nginx_version) && ((nginx_version) >= 1003010 || (nginx_version) >= 1002007 && (nginx_version) < 1003000)
 	ngx_gateway_log_buf_t			*buffer;
 #endif
 	ngx_gateway_log_src_conf_t 		*lscf;
 	ngx_gateway_core_srv_conf_t		*cscf;

 	ngx_log_debug0(NGX_LOG_DEBUG_GATEWAY, s->ngx_connection_t->log, 0,
 					"gateway access log handler");

 	cscf = ngx_gateway_get_module_srv_conf(s, ngx_gateway_core_module);
 	lscf = cscf->access_log;

 	if (lscf->off) {
 		return NGX_OK;
 	}

 	c = s->connection;
 	log = lscf->logs->elts;
 	for (l = 0; l < lscf->logs->nelts; ++l)
 	{
 		if (ngx_time() == log[l].disk_full_time) {
 			continue;
 		}

 		len = 0;

 		len += sizeof("1970/09/28 12:00:00");		/* log time */
 		len += NGX_INT64_LEN + 2;					/* [ngx_pid] */
 		len += c->addr_text.len + 1;				/* client address */
 		len += s->addr_text->len + 1;				/* this session address */
 		len += sizeof("1970/09/28 12:00:00");		/* accept time */
 		len += sizeof("255.255.255.255:65536");		/* upstream address */
 		len += NGX_OFF_T_LEN + 1;					/* read bytes from client */
 		len += NGX_OFF_T_LEN + 1;					/* write bytes to client */
 		len += NGX_LINEFEED_SIZE;					/*  */

 		file = log[l].file;

 #if defined(nginx_version) && ((nginx_version) >= 1003010 || (nginx_version) >= 1002007 && (nginx_version) < 1003000)
 		if (file && file->data) {

 			buffer = file->data;

 			if (len > (size_t)(buffer->last - buffer->pos)) {
 				ngx_gateway_log_write(s, &log[l], buffer->start, buffer->pos - buffer->start);
 				buffer->pos = buffer->start;
 			}

 			if (len <= (size_t)(buffer->last - buffer->pos)) {

 				p = buffer->pos;

 				p = ngx_gateway_log_fill(s, p);

 				buffer->pos = p;

 				continue;
 			}
 		}
 #else
 		if (file && file->buffer) {

 			if (len > (size_t)(file->last - file->pos)) {
 				ngx_gateway_log_write(s, &log[l], file->buffer, file->last - file->buffer);

 				file->pos = file->buffer;
 			}

 			if (len <= (size_t)(file->last - file->pos)) {

 				p = file->pos;

 				p = ngx_gateway_log_fill(s, p);

 				file->pos = p;

 				continue;
 			}
 		}
#endif	

		line = ngx_pnalloc(s->pool, len);
		if (NULL != line) {
			return NGX_ERROR;
		}	

		p = line;

		p = ngx_gateway_log_fill(s, p);

		ngx_gateway_log_write(s, &log[l], line, p - line);
 	}

 	return NGX_OK;
 }

 static u_char *
 ngx_gateway_time(u_char *buf, time_t t)
 {
 	ngx_tm_t 			tm;

 	ngx_localtime(t, &tm);

 	return ngx_sprintf(buf, "%4d/%02d/%02d %02d:%02d:%02d",
 						tm.ngx_tm_year, tm.ngx_tm_mon,
 						tm.ngx_tm_mday, tm.ngx_tm_hour,
 						tm.ngx_tm.min, tm.ngx_tm_sec);
 }

 static u_char *
 ngx_gateway_log_fill(ngx_gateway_session_t *s, u_char *buf)
 {
 	u_char					*last;
 	ngx_str_t				*name;
 	ngx_connection_t		*c;
 	ngx_gateway_upstream_t	*u;

 	c = s->connection;

 	last = ngx_cpymem(buf, ngx_cached_err_log_time.data, ngx_cached_err_log_time.len);

 	last = ngx_sprintf(last, " [%P]", ngx_pid);
 	last = ngx_sprintf(last, " %V", &c->addr_text);
 	last = ngx_sprintf(last, " %V", s->addr_text);
 	last = ngx_gateway_time(last, s->start_sec);

 	name = NULL;
 	if (s->upstream) {
 		u = s->upstream;
 		if (u->peer.connection) {
 			name = u->peer.name;
 		}
 	}

 	if (name) {
 		last = ngx_sprintf(last, " %V", name);
 	} else {
 		last = ngx_sprintf(last, " -");
 	}

 	last = ngx_sprintf(last, " %O", s->bytes_read);
 	last = ngx_sprintf(last, " %O", s->bytes_write);

 	ngx_linefeed(last);

 	return last;
 }

 static void 
 ngx_gateway_log_write(ngx_gateway_session_t *s, ngx_gateway_log_t *log, u_char *buf, size_t len)
 {
 	u_char							*name;
 	time_t							now;
 	ssize_t							n;
 	ngx_err_t 						err;

 	if (0 == len) return;

 	name = log->file->name.data;
 	n = ngx_write_fd(log->file->fd, buf, len);

 	if (n == (ssize_t) len) {
 		return;
 	}

 	now = ngx_time();

 	if (-1 == n) {
 		err = ngx_errno;

 		if (NGX_ENOSPC == err) {
 			log->disk_full_time = now;
 		}

 		if (now -log->error_log_time > 59) {
 			ngx_log_error(NGX_LOG_ALERT, s->connection->log, err, 
 						ngx_write_fd_n " to \"%s\" failed", name);

 			log->error_log_time = now;
 		}
 	}

 	if (now -log->error_log_time > 59) {
 		ngx_log_error(NGX_LOG_ALERT, s->connection->log, err, 
 						ngx_write_fd_n " to \"%s\" was incomplete: %Z of %uz",
 						name, n, len);

 		log->error_log_time = now;
 	}
 }

