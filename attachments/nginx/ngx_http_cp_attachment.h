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

#ifndef __NGX_HTTP_CP_ATTACHMENT_H__
#define __NGX_HTTP_CP_ATTACHMENT_H__

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "abstruct_ipc.h"

typedef enum ngx_http_cp_verdict
{
    TRAFFIC_VERDIT_INSPECT,
    TRAFFIC_VERDIT_DROP,
    TRAFFIC_VERDIT_ACCEPT,
    TRAFFIC_VERDIT_IRRELEVANT
} ngx_http_cp_verdict_e;

typedef enum ngx_http_chunk_type
{
    REQUEST_START,
    REQUEST_HEADER,
    REQUEST_BODY,
    REQUEST_END,
    RESPONSE_CODE,
    RESPONSE_HEADER,
    RESPONSE_BODY,
    RESPONSE_END
} ngx_http_chunk_type_e;

typedef struct {
    ngx_flag_t enable;
} ngx_cp_attachment_conf_t;

typedef struct {
    uint32_t request_id;
    ngx_http_cp_verdict_e verdict;
} ngx_http_cp_session_data_t;


extern ngx_module_t ngx_http_cp_attachment_module;
extern AbstractIpc *ngx_http_cp_ipc_channel;

ngx_int_t
ngx_cp_attachment_postconfiguration(
    ngx_conf_t *conf
);

ngx_int_t
ngx_http_cp_meta_data_sender(
    ngx_http_request_t *request,
    uint32_t cur_request_id
);

ngx_int_t
ngx_http_cp_res_code_sender(
    ngx_http_request_t *request,
    uint32_t cur_request_id
);

ngx_int_t
ngx_http_cp_header_sender(
    ngx_list_part_t *headers_list,
    ngx_http_chunk_type_e header_type,
    uint32_t cur_request_id
);

ngx_int_t
ngx_http_cp_body_sender(
    ngx_chain_t *input,
    ngx_http_chunk_type_e body_type,
    uint32_t cur_request_id
);

ngx_int_t
ngx_http_cp_end_message_sender(
    ngx_http_chunk_type_e end_transaction_type,
    uint32_t cur_request_id
);

ngx_int_t
ngx_http_cp_reply_receiver(
    ngx_http_cp_session_data_t *session_data_p,
    ngx_int_t expected_replies
);

#endif // __NGX_HTTP_CP_ATTACHMENT_H__