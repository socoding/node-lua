#include "common.h"
#include "utils.h"

char* filter_arg(char **param, int32_t *len) {
	char *begptr = *param;
	if (begptr == NULL) return NULL;

	while (*begptr != '\0' && (*begptr == ' ' || *begptr == '\t')) {
		++begptr;
	}
	char *endptr = begptr;
	while (*endptr != '\0' && (*endptr != ' ' && *endptr != '\t')) {
		++endptr;
	}
	if (begptr != endptr) {
		if (*endptr != '\0') {  /* don't reach the end */
			*param = endptr + 1;
			*endptr = '\0';
		} else {				/* reach the end */
			*param = NULL;
		}
		if (len) {
			*len = endptr - begptr;
		}
		return begptr;
	} else {					/* reach the end */
		*param = NULL;
		return NULL;
	}
}

void* nl_memdup(const void* src, uint32_t len)
{
	if (src) {
		if (len != 0) {
			void* temp = nl_malloc(len);
			if (temp) {
				return memcpy(temp, src, len);
			}
		} else {
			return nl_malloc(1);
		}
	}
	return NULL;
}

bool socket_host(uv_os_sock_t sock, bool local, char* host, uint32_t host_len, bool* ipv6, uint16_t* port)
{
	union sock_name_u {
		sockaddr sock;
		sockaddr_in sock4;
		sockaddr_in6 sock6;
	} sock_name;
	int sock_len = sizeof(sock_name);
#ifdef _WIN32
	int result = local ? getsockname(sock, (sockaddr*)&sock_name, &sock_len) : getpeername(sock, (sockaddr*)&sock_name, &sock_len);
#else
	int result = local ? getsockname(sock, (sockaddr*)&sock_name, (socklen_t*)&sock_len) : getpeername(sock, (sockaddr*)&sock_name, (socklen_t*)&sock_len);
#endif
	if (result != 0)
		return false;
	uint16_t family = sock_name.sock.sa_family;
	if (family == AF_INET) {
		if (host != NULL) {
			uv_ip4_name(&sock_name.sock4, host, host_len);
		}
		if (ipv6 != NULL) {
			*ipv6 = false;
		}
		if (port != NULL) {
			*port = ntohs(sock_name.sock4.sin_port);
		}
		return true;
	}
	if (family == AF_INET6) {
		if (host != NULL) {
			uv_ip6_name(&sock_name.sock6, host, host_len);
		}
		if (ipv6 != NULL) {
			*ipv6 = true;
		}
		if (port != NULL) {
			*port = ntohs(sock_name.sock6.sin6_port);
		}
		return true;
	}
	return false;
}

bool sockaddr_host(struct sockaddr* addr, char* host, uint32_t host_len, bool* ipv6, uint16_t* port)
{
	uint16_t family = addr->sa_family;
	if (family == AF_INET) {
		if (host != NULL) {
			uv_ip4_name((sockaddr_in*)addr, host, host_len);
		}
		if (ipv6 != NULL) {
			*ipv6 = false;
		}
		if (port != NULL) {
			*port = ntohs(((sockaddr_in*)addr)->sin_port);
		}
		return true;
	}
	if (family == AF_INET6) {
		if (host != NULL) {
			uv_ip6_name((sockaddr_in6*)addr, host, host_len);
		}
		if (ipv6 != NULL) {
			*ipv6 = true;
		}
		if (port != NULL) {
			*port = ntohs(((sockaddr_in6*)addr)->sin6_port);
		}
		return true;
	}
	return false;
}

uv_err_code host_sockaddr(bool ipv6, const char* host, uint16_t port, struct sockaddr* addr)
{
	if (!ipv6) {
		struct sockaddr_in* addr4 = (struct sockaddr_in*)addr;
		memset(addr4, 0, sizeof(struct sockaddr_in));
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(port);
		return uv_inet_pton(AF_INET, host, (void*)&addr4->sin_addr).code;
	} else {
		struct sockaddr_in6* addr6 = (struct sockaddr_in6*)addr;
		memset(addr6, 0, sizeof(struct sockaddr_in6));
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(port);
		return uv_inet_pton(AF_INET6, host, (void*)&addr6->sin6_addr).code;
	}
}