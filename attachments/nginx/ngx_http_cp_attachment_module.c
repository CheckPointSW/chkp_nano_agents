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

#define JOIN_IPC 0

static void *
ngx_cp_attachment_create_loc_conf(ngx_conf_t *conf)
{
    ngx_cp_attachment_conf_t  *module_conf;

    module_conf = ngx_pcalloc(conf->pool, sizeof(ngx_cp_attachment_conf_t));
    if (module_conf != NULL) {
        module_conf->enable = NGX_CONF_UNSET;
    }

    return module_conf;
}

static char *
ngx_cp_attachment_merge_loc_conf(ngx_conf_t *configuration, void *curr, void *next)
{
    (void)configuration;
    ngx_cp_attachment_conf_t *prev = curr;
    ngx_cp_attachment_conf_t *conf = next;

     ngx_conf_merge_value(conf->enable, prev->enable, 0);

    return NGX_CONF_OK;
}

static ngx_http_module_t ngx_cp_attachment_module_ctx = {
    NULL,
    ngx_cp_attachment_postconfiguration,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_cp_attachment_create_loc_conf,
    ngx_cp_attachment_merge_loc_conf
};

static ngx_command_t ngx_cp_attachment_commands[] = {
    {
        ngx_string("cp-nano-nginx-attachment"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_cp_attachment_conf_t, enable),
        NULL
    },
    ngx_null_command
};

static ngx_int_t
ngx_cp_attachment_init_process(ngx_cycle_t *cycle)
{
    (void)cycle;

    ngx_http_cp_ipc_channel = initIpc(JOIN_IPC);

    return NGX_OK;
}

static void
ngx_cp_attachment_exit_process(ngx_cycle_t *cycle)
{
    (void)cycle;

    finiIpc(ngx_http_cp_ipc_channel);
}

ngx_module_t ngx_http_cp_attachment_module = {
    NGX_MODULE_V1,
    &ngx_cp_attachment_module_ctx,
    ngx_cp_attachment_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    &ngx_cp_attachment_init_process,
    NULL,
    NULL,
    &ngx_cp_attachment_exit_process,
    NULL,
    NGX_MODULE_V1_PADDING
};