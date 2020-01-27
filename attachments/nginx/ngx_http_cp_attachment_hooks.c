/*
*   Copyright 2020 Check Point Software Technologies LTD
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*       you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/

#include "ngx_http_cp_attachment.h"

static ngx_http_request_body_filter_pt ngx_http_next_request_body_filter;
static ngx_http_output_body_filter_pt  ngx_http_next_response_body_filter;

static ngx_int_t
ngx_http_cp_req_header_handler(ngx_http_request_t *request)
{
    ngx_cp_attachment_conf_t *conf;
    ngx_http_cp_session_data_t *session_data_p;
    ngx_int_t messages_pending_reply;
    static uint32_t request_id = 0;

    conf = ngx_http_get_module_loc_conf(request, 
                                        ngx_http_cp_attachment_module);
    if (conf && conf->enable) {
        return NGX_OK;
    }

    session_data_p = (ngx_http_cp_session_data_t *)ngx_pcalloc(
        request->pool,
        sizeof(ngx_http_cp_session_data_t)
    );
    if (!session_data_p) {
        return NGX_ERROR;
    }
    session_data_p->verdict = TRAFFIC_VERDIT_INSPECT;
    session_data_p->request_id = ++request_id;
    ngx_http_set_ctx(request, session_data_p, ngx_http_cp_attachment_module);

    if (ngx_http_cp_meta_data_sender(request,
                                     session_data_p->request_id) < 0) {
        return NGX_ERROR;
    }
    messages_pending_reply = ngx_http_cp_header_sender(
        &(request->headers_in.headers.part),
        REQUEST_HEADER,
        session_data_p->request_id
    );
    if (messages_pending_reply < 0) {
        return NGX_ERROR;
    }
    ++messages_pending_reply;
    if (request->headers_in.content_length_n == (off_t)(-1) &&
        !(request->headers_in.chunked)) {
        if (ngx_http_cp_end_message_sender(REQUEST_END,
                                           session_data_p->request_id) < 0) {
            return NGX_ERROR;
        }
        ++messages_pending_reply;
    }

    if (ngx_http_cp_reply_receiver(session_data_p,
                                   messages_pending_reply) != NGX_OK) {
        request->keepalive = 0;
        return NGX_HTTP_FORBIDDEN;
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_cp_req_body_filter(ngx_http_request_t *request, ngx_chain_t *input)
{
    ngx_http_cp_session_data_t *session_data_p;
    ngx_int_t messages_pending_reply;

    session_data_p = (ngx_http_cp_session_data_t *)ngx_http_get_module_ctx(
        request, 
        ngx_http_cp_attachment_module
    );
    if (session_data_p &&
        session_data_p->verdict == TRAFFIC_VERDIT_INSPECT &&
        input) {
        messages_pending_reply = ngx_http_cp_body_sender(
            input,
            REQUEST_BODY,session_data_p->request_id
        );
        if (messages_pending_reply < 0) {
            return NGX_ERROR;
        }
        if (ngx_http_cp_end_message_sender(REQUEST_END,
                                           session_data_p->request_id) < 0) {
            return NGX_ERROR;
        }
        ++messages_pending_reply;

        if (ngx_http_cp_reply_receiver(session_data_p,
                                       messages_pending_reply) != NGX_OK) {
            request->keepalive = 0;
            return NGX_HTTP_FORBIDDEN;
        }
    }

    return ngx_http_next_request_body_filter(request, input);
}

static ngx_int_t
ngx_http_cp_res_header_handler(ngx_http_request_t *request)
{
    ngx_http_cp_session_data_t *session_data_p;
    ngx_int_t messages_pending_reply;

    session_data_p = (ngx_http_cp_session_data_t *)ngx_http_get_module_ctx(
        request,
        ngx_http_cp_attachment_module
    );
    if (session_data_p != NULL &&
        session_data_p->verdict == TRAFFIC_VERDIT_INSPECT) {
        if (ngx_http_cp_res_code_sender(request,
                                        session_data_p->request_id) < 0) {
            return NGX_ERROR;
        }

        messages_pending_reply = ngx_http_cp_header_sender(
            &(request->headers_out.headers.part),
            RESPONSE_HEADER,
            session_data_p->request_id
        );
        if (messages_pending_reply < 0) {
            return NGX_ERROR;
        }
        ++messages_pending_reply;

        if (ngx_http_cp_reply_receiver(session_data_p,
                                       messages_pending_reply) != NGX_OK) {
            request->keepalive = 0;
            return NGX_HTTP_FORBIDDEN;
        }
    }
    return NGX_OK;
}

static ngx_int_t
ngx_http_cp_res_body_filter(ngx_http_request_t *request, ngx_chain_t *input)
{
    ngx_http_cp_session_data_t *session_data_p;
    ngx_int_t messages_pending_reply;

    session_data_p = (ngx_http_cp_session_data_t *)ngx_http_get_module_ctx(
        request,
        ngx_http_cp_attachment_module
    );

    if (session_data_p &&
        session_data_p->verdict == TRAFFIC_VERDIT_INSPECT &&
        input) {
        messages_pending_reply = ngx_http_cp_body_sender(
            input,
            RESPONSE_BODY,
            session_data_p->request_id
        );
        if (messages_pending_reply < 0) {
            return NGX_ERROR;
        }
        if (ngx_http_cp_end_message_sender(RESPONSE_END,
                                           session_data_p->request_id) < 0) {
            return NGX_ERROR;
        }
        ++messages_pending_reply;

        if (ngx_http_cp_reply_receiver(session_data_p,
                                       messages_pending_reply) != NGX_OK) {
            request->keepalive = 0;
            return NGX_HTTP_FORBIDDEN;
        }
    }

    return ngx_http_next_response_body_filter(request, input);
}

ngx_int_t
ngx_cp_attachment_postconfiguration(ngx_conf_t *conf)
{
    ngx_http_core_main_conf_t *core_main_conf;
    ngx_http_handler_pt       *handler;

    core_main_conf = ngx_http_conf_get_module_main_conf(
        conf,
        ngx_http_core_module
    );

    handler = ngx_array_push(
        &core_main_conf->phases[NGX_HTTP_ACCESS_PHASE].handlers
    );
    if (handler == NULL) {
        return NGX_ERROR;
    }
    *handler = ngx_http_cp_req_header_handler;

    ngx_http_next_request_body_filter = ngx_http_top_request_body_filter;
    ngx_http_top_request_body_filter = ngx_http_cp_req_body_filter;

    handler = ngx_array_push(
        &core_main_conf->phases[NGX_HTTP_CONTENT_PHASE].handlers
    );
    if (handler == NULL) {
        return NGX_ERROR;
    }
    *handler = ngx_http_cp_res_header_handler;

    ngx_http_next_response_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_cp_res_body_filter;

    return NGX_OK;
}