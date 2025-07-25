#include "server.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

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

int server_waitclient(struct server *server, struct client* client) {
	socklen_t clen = sizeof client->addr;
	return ((client->fd = accept(server->fd, (void*)&client->addr, &clen)) == -1)*-1;
}

void set_socket_options(int fd) {
	int val = 4 * 1024 * 1024;
	if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &val, sizeof(int)) < 0) {
		perror("setsockopt SO_SNDBUF");
	}
	if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(int)) < 0) {
		perror("setsockopt SO_RCVBUF");
	}
	val = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(int)) < 0) {
		perror("setsockopt KEEPALIVE");
	}
	val = 3;
	if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(int)) < 0) {
		perror("setsockopt TCP_KEEPCNT");
	}
	val = 60;
	if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(int)) < 0) {
		perror("setsockopt TCP_KEEPIDLE");
	}
	val = 30;
	if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(int)) < 0) {
		perror("setsockopt TCP_KEEPINTVL");
	}
	val = 1;
	if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(int)) < 0) {
		perror("setsockopt TCP_NODELAY");
	}
}

int server_setup(struct server *server, const char* listenip, unsigned short port) {
	struct addrinfo *ainfo = 0;
	if(resolve(listenip, port, &ainfo)) return -1;
	struct addrinfo* p;
	int listenfd = -1;
	for(p = ainfo; p; p = p->ai_next) {
		if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
			continue;
		int yes = 1;
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if(bind(listenfd, p->ai_addr, p->ai_addrlen) < 0) {
			close(listenfd);
			listenfd = -1;
			continue;
		}
		break;
	}
	freeaddrinfo(ainfo);
	if(listenfd < 0) return -2;
	set_socket_options(listenfd);
	if(listen(listenfd, SOMAXCONN) < 0) {
		close(listenfd);
		return -3;
	}
	server->fd = listenfd;
	return 0;
}

int server_connect(const char* connectip, unsigned short port) {
	struct addrinfo *ainfo = 0;
	if(resolve(connectip, port, &ainfo)) return 1;
	struct addrinfo* p;
	int fd = -1;
	for(p = ainfo; p; p = p->ai_next) {
		fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(fd < 0) {
			perror("socket");
			continue;
		}
		set_socket_options(fd);
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
