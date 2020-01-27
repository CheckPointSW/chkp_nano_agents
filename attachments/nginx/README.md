# NGINX Attachment

## Description
This is a plugin to NGINX server. The purpose of the plugin is to intercept the traffic passing through the NGIXN server and send it for inspection by the Infinity Next services.

## Attachment inputs
HTTP traffic, parsed by the NGINX web server. 

## Attachment outputs
Interjection of the incoming input and injection capabilities to it. Ability to either accept, block, or manipulate (inject for example) the inputs.

## Compilation
In order to compile the plugin, you'll need to create a shared object from the sources in this library. This will require headers from the NGINX version that you want to load the attachment to, as well as headers from the `core_components\include` directory.

Here is an example of such such compilation line may look like:
```
gcc -fpic -shared \
    -I core\include \
    -I <path to NGIXN headers> \
    ngx_http_cp_attachment_module.c \
    ngx_http_cp_attachment_hooks.c \ 
    ngx_http_cp_attachment_communication.c \
    -o ngx_http_cp_attachment.so
```
