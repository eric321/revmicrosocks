#include "server.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int resolve(const char *host, unsigned short port, struct addrinfo** addr) {
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_PASSIVE,
	};
	char port_buf[8];
	snprintf(port_buf, sizeof port_buf, "%u", port);
	return getaddrinfo(host, port_buf, &hints, addr);
}

int resolve_sa(const char *host, unsigned short port, union sockaddr_union *res) {
	struct addrinfo *ainfo = 0;
	int ret;
	SOCKADDR_UNION_AF(res) = AF_UNSPEC;
	if((ret = resolve(host, port, &ainfo))) return ret;
	memcpy(res, ainfo->ai_addr, ainfo->ai_addrlen);
	freeaddrinfo(ainfo);
	return 0;
}

int bindtoip(int fd, union sockaddr_union *bindaddr) {
	socklen_t sz = SOCKADDR_UNION_LENGTH(bindaddr);
	if(sz)
		return bind(fd, (struct sockaddr*) bindaddr, sz);
	return 0;
}

int do_connect(struct server *server) {
	struct addrinfo *ainfo = 0;
	if(resolve(server->ip, server->port, &ainfo)) return 1;
	struct addrinfo* p;
	int fd = -1;
	for(p = ainfo; p; p = p->ai_next) {
		if((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			perror("socket");
			continue;
		}
		int val = 4 * 1024 * 1024;
		if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &val, sizeof(int)) < 0) {
			perror("setsockopt SO_SNDBUF");
		}
		if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(int)) < 0) {
			perror("setsockopt SO_RCVBUF");
		}
		if(connect(fd, p->ai_addr, p->ai_addrlen) < 0) {
			perror("connect");
			close(fd);
			fd = -1;
			continue;
		}
		break;
	}
	freeaddrinfo(ainfo);
	return fd;
}

int server_waitclient(struct server *server, struct client* client) {
	int sleeptime = 1;
	for(;;) {
		if((client->fd = do_connect(server)) >= 0) {
			return 0;
		}
		sleep(sleeptime);
		sleeptime *= 2;
		if(sleeptime > 300) sleeptime = 300;
	}
}

int server_setup(struct server *server, const char* connectip, unsigned short port) {
	server->ip = connectip;
	server->port = port;
	return 0;
}
