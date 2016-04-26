#ifndef UTILS_H_
#define UTILS_H_
#include "common.h"

extern char* filter_arg(char **param, int32_t *len);
extern void* nl_memdup(const void* src, uint32_t len);
extern bool socket_host(uv_os_sock_t sock, bool local, char* host, uint32_t host_len, bool* ipv6, uint16_t* port);
extern bool sockaddr_host(struct sockaddr* addr, char* host, uint32_t host_len, bool* ipv6, uint16_t* port);
extern uv_err_code host_sockaddr(bool ipv6, const char* host, uint16_t port, struct sockaddr* addr);
#endif
