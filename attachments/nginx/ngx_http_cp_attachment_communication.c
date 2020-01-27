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

AbstractIpc *ngx_http_cp_ipc_channel;

static const void *
ngx_http_cp_convert_sockaddr_to_str(
    const struct sockaddr *sa,
    u_char *ip_addr
)
{
    void *ip;
    if (sa->sa_family == AF_INET) {
        ip = (void *)&(((struct sockaddr_in*)sa)->sin_addr);
    } else {
        ip = (void *)&(((struct sockaddr_in6*)sa)->sin6_addr);
    }

    return inet_ntop(AF_INET, ip, (char *)ip_addr, INET6_ADDRSTRLEN);
}

static uint16_t
ngx_http_cp_get_source_port(const struct sockaddr *sa)
{
    uint16_t port;
    if (sa->sa_family == AF_INET) {
        port = ((struct sockaddr_in*)sa)->sin_port;
    } else {
        port = ((struct sockaddr_in6*)sa)->sin6_port;
    }
    return htons(port);
}

ngx_int_t
ngx_http_cp_meta_data_sender(
    ngx_http_request_t *request,
    uint32_t cur_request_id
)
{
    ngx_http_chunk_type_e request_type;
    struct sockaddr *sockaddr;
    u_char client_ip[INET6_ADDRSTRLEN];
    uint16_t client_ip_len;
    uint16_t client_port;
    struct sockaddr *local_sockaddr;
    u_char listening_ip[INET6_ADDRSTRLEN];
    uint16_t listening_ip_len;
    uint16_t listening_port;
    u_char *buffers[16];
    uint16_t buffer_sizes[16];

    request_type = REQUEST_START;
    buffers[0] = (u_char *)&request_type;
    buffer_sizes[0] = sizeof(request_type);

    buffers[1] = (u_char *)&cur_request_id;
    buffer_sizes[1] = sizeof(cur_request_id);

    sockaddr = request->connection->sockaddr;
    if (!ngx_http_cp_convert_sockaddr_to_str(sockaddr, client_ip)) {
        return -1;
    }
    client_ip_len = strlen((char*)client_ip);
    buffers[2] = (u_char *)&client_ip_len;
    buffer_sizes[2] = sizeof(client_ip_len);
    buffers[3] = client_ip;
    buffer_sizes[3] = client_ip_len;

    client_port = ngx_http_cp_get_source_port(sockaddr);
    buffers[4] = (u_char *)&client_port;
    buffer_sizes[4] = sizeof(client_port);

    local_sockaddr = request->connection->local_sockaddr;
    if (!ngx_http_cp_convert_sockaddr_to_str(local_sockaddr, listening_ip)) {
        return -1;
    }
    listening_ip_len = strlen((char *)listening_ip);
    buffers[5] = (u_char *)&listening_ip_len;
    buffer_sizes[5] = sizeof(listening_ip_len);
    buffers[6] = listening_ip;
    buffer_sizes[6] = listening_ip_len;

    listening_port = ngx_http_cp_get_source_port(local_sockaddr);
    buffers[7] = (u_char *)&listening_port;
    buffer_sizes[7] = sizeof(listening_port);

    buffers[8] = (u_char *)&(request->http_protocol.len);
    buffer_sizes[8] = sizeof(request->http_protocol.len);
    buffers[9] = request->http_protocol.data;
    buffer_sizes[9] = request->http_protocol.len;

    buffers[10] = (u_char *)&(request->method_name.len);
    buffer_sizes[10] = sizeof(request->method_name.len);
    buffers[11] = request->method_name.data;
    buffer_sizes[11] = request->method_name.len;

    buffers[12] = (u_char *)&(request->headers_in.host->value.len);
    buffer_sizes[12] = sizeof(request->headers_in.host->value.len);
    buffers[13] = request->headers_in.host->value.data;
    buffer_sizes[13] = request->headers_in.host->value.len;

    buffers[14] = (u_char *)&(request->unparsed_uri.len);
    buffer_sizes[14] = sizeof(request->unparsed_uri.len);
    buffers[15] = request->unparsed_uri.data;
    buffer_sizes[15] = request->unparsed_uri.len;

    if (!sendChunkedData(ngx_http_cp_ipc_channel,
                         buffer_sizes,
                         buffers,
                         16)) {
        return -1;
    }

    return 1;
}

ngx_int_t
ngx_http_cp_res_code_sender(
    ngx_http_request_t *request,
    uint32_t cur_request_id
)
{
    ngx_http_chunk_type_e request_type;
    uint8_t status;
    u_char *buffers[3];
    uint16_t buffer_sizes[3];

    request_type = RESPONSE_CODE;
    buffers[0] = (u_char *)&request_type;
    buffer_sizes[0] = sizeof(request_type);

    buffers[1] = (u_char *)&cur_request_id;
    buffer_sizes[1] = sizeof(cur_request_id);

    status = request->headers_out.status;
    buffers[2] = (u_char *)&status;
    buffer_sizes[2] = sizeof(status);

    if (!sendChunkedData(ngx_http_cp_ipc_channel,
                         buffer_sizes,
                         buffers,
                         3)) {
        return -1;
    }

    return 1;
}

ngx_int_t
ngx_http_cp_header_sender(
    ngx_list_part_t *headers_list,
    ngx_http_chunk_type_e header_type,
    uint32_t cur_request_id
)
{
    ngx_list_part_t *headers_iter;
    ngx_uint_t header_idx;
    ngx_table_elt_t *header;
    ngx_int_t num_of_headers_sent;
    u_char *buffers[6];
    uint16_t buffer_sizes[6];

    buffers[0] = (u_char *)&header_type;
    buffer_sizes[0] = sizeof(header_type);

    buffers[1] = (u_char *)&cur_request_id;
    buffer_sizes[1] = sizeof(cur_request_id);

    num_of_headers_sent = 0;
    for (headers_iter = headers_list;
         headers_iter;
         headers_iter = headers_iter->next) {
        for (header_idx = 0;
             header_idx < headers_iter->nelts;
             ++header_idx) {
            header = (ngx_table_elt_t *)(headers_iter->elts) + header_idx;

            buffers[2] = (u_char *)&(header->key.len);
            buffer_sizes[2] = sizeof(header->key.len);
            buffers[3] = header->key.data;
            buffer_sizes[3] = header->key.len;

            buffers[4] = (u_char *)&(header->value.len);
            buffer_sizes[4] = sizeof(header->value.len);
            buffers[5] = header->value.data;
            buffer_sizes[5] = header->value.len;

            if (!sendChunkedData(ngx_http_cp_ipc_channel,
                                 buffer_sizes,
                                 buffers,
                                 6)) {
                return -1;
            }

            ++num_of_headers_sent;
        }
    }

    return num_of_headers_sent;
}

ngx_int_t
ngx_http_cp_body_sender(
    ngx_chain_t *input,
    ngx_http_chunk_type_e body_type,
    uint32_t cur_request_id
)
{
    ngx_chain_t *chain_iter;
    ngx_int_t parts_send;
    ngx_buf_t *buf;
    u_char *buffers[3];
    uint16_t buffer_sizes[3];

    buffers[0] = (u_char *)&body_type;
    buffer_sizes[0] = sizeof(body_type);

    buffers[1] = (u_char *)&cur_request_id;
    buffer_sizes[1] = sizeof(cur_request_id);

    parts_send = 0;
    for (chain_iter = input; chain_iter; chain_iter = chain_iter->next) {
        buf = chain_iter->buf;

        buffers[2] = buf->start;
        buffer_sizes[2] = buf->end - buf->start;

        if (!sendChunkedData(ngx_http_cp_ipc_channel,
                             buffer_sizes,
                             buffers,
                             3)) {
            return -1;
        }

        ++parts_send;
    }

    return parts_send;
}

ngx_int_t
ngx_http_cp_end_message_sender(
    ngx_http_chunk_type_e end_transaction_type,
    uint32_t cur_request_id
)
{
    u_char *buffers[2];
    uint16_t buffer_sizes[2];

    buffers[0] = (u_char *)&end_transaction_type;
    buffer_sizes[0] = sizeof(end_transaction_type);

    buffers[1] = (u_char *)&cur_request_id;
    buffer_sizes[1] = sizeof(cur_request_id);

    if (!sendChunkedData(ngx_http_cp_ipc_channel, buffer_sizes, buffers, 3)) {
        return -1;
    }

    return 1;
}

ngx_int_t
ngx_http_cp_reply_receiver(
    ngx_http_cp_session_data_t *session_data_p,
    ngx_int_t expected_replies
)
{
    ngx_int_t reply_idx;
    ReceivedData *reply;
    ngx_http_cp_verdict_e verdict;

    reply = NULL;
    for (reply_idx = 0; reply_idx < expected_replies; ++reply_idx) {
        if (!receiveData(ngx_http_cp_ipc_channel, reply)) {
            return NGX_ERROR;
        }

        verdict = ((ngx_http_cp_session_data_t *)(reply->buffer))->verdict;

        releaseData(reply);

        if (verdict == TRAFFIC_VERDIT_DROP) {
            session_data_p->verdict = verdict;
            return NGX_ERROR;
        }
        if (verdict == TRAFFIC_VERDIT_ACCEPT) {
            session_data_p->verdict = verdict;
            return NGX_OK;
        }
    }

    return NGX_OK;
}