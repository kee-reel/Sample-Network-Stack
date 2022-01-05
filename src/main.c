#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BACKLOG 10

#define ERR_LOG(status) \
	g_status = status; \
	if(g_status < 0) { \
		int t = errno; \
		fprintf(stderr, "[ERROR] %s:%s():%d: %s - %s\n", __FILE__, __func__, __LINE__, #status, strerror(t)); \
	}

#define ERR_EXIT(status) \
	g_status = status; \
	if(g_status < 0) { \
		int t = errno; \
		fprintf(stderr, "[ERROR] %s:%s():%d: %s - %s\n", __FILE__, __func__, __LINE__, #status, strerror(t)); \
		exit(2); \
	}

int g_status = 0;
char g_is_server = 0;
char g_is_tcp = 0;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void show_conn_ip(struct sockaddr_storage *conn_ai)
{
	char s[INET6_ADDRSTRLEN];
	inet_ntop(conn_ai->ss_family, get_in_addr((struct sockaddr *)&conn_ai), s, sizeof s);
	printf("[=] Got connection from %s\n", s);
}

void print_buf(char *name, char *buf, int len)
{
	if(len > 0)
	{
		printf("[%s]:", name);
		for(int i = 0; i < len; ++i)
			putc(buf[i], stdout);
		putc('\n', stdout);
	}
}

void client_udp(int fd, struct addrinfo *ai)
{
	int status;
	char buf[100];
	do
	{
		fgets(buf, 100, stdin);
		ERR_LOG(status = sendto(fd, buf, strlen(buf), 0, ai->ai_addr, ai->ai_addrlen));
		print_buf("SEND", buf, strlen(buf));
	}
	while(status >= 0);
}

void client_tcp(int fd, struct addrinfo *ai)
{
	int yes = 1;
	ERR_EXIT(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes));
	ERR_EXIT(listen(fd, BACKLOG));

	int status;
	char buf[100];

	ERR_EXIT(connect(fd, ai->ai_addr, ai->ai_addrlen));
	do
	{
		fgets(buf, 100, stdin);
		ERR_LOG(status = send(fd, buf, strlen(buf), 0));
		print_buf("SEND", buf, strlen(buf));
	}
	while(status >= 0);
}

void server_udp(int fd, struct addrinfo *ai)
{
	int len;
	struct sockaddr_storage conn_ai;
	int conn_fd, conn_ai_len = sizeof conn_ai, recv_len;
	char buf[100];
	while(1)
	{
		ERR_LOG(len = recvfrom(fd, buf, 100, 0, (struct sockaddr *)&conn_ai, &conn_ai_len));
		show_conn_ip(&conn_ai);
		print_buf("RECV", buf, len);
	}
}

void server_tcp(int fd, struct addrinfo *ai)
{
	struct sockaddr_storage conn_ai;
	int conn_fd, conn_ai_len = sizeof conn_ai, len;
	char buf[100];
	while(1)
	{
		ERR_LOG(conn_fd = accept(fd, (struct sockaddr *)&conn_ai, &conn_ai_len));
		if(conn_fd < 0)
			continue;

		show_conn_ip(&conn_ai);
		do
		{
			ERR_LOG(len = recv(conn_fd, buf, 100, 0));
			print_buf("RECV", buf, len);
		}
		while(len > 0);
	}
}

int main(int argc, char *argv[])
{
	if(argc < 2 || (strcmp(argv[1], "-s") != 0 && strcmp(argv[1], "-c") != 0) ||
			(strcmp(argv[2], "-t") != 0 && strcmp(argv[2], "-u") != 0))
	{
		fprintf(stderr, "Usage:\nFor server: %s -s (-t | -u) PORT\nFor client: %s -c (-t | -u) HOST PORT\n", argv[0], argv[0]);
		return 1;
	}

	g_is_server = strcmp(argv[1], "-s") == 0;
	g_is_tcp = strcmp(argv[2], "-t") == 0;

	struct addrinfo hints, *ai;
	char *host = NULL, *port = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Don't care. To use specific: AF_INET (IPv4) or AF_INET6 (IPv6)
	hints.ai_socktype = g_is_tcp ? SOCK_STREAM : SOCK_DGRAM;
	if(g_is_server)
	{
		port = argv[3];
		hints.ai_flags = AI_PASSIVE;
	}
	else
	{
		host = argv[3];
		port = argv[4];
	}

	int status = getaddrinfo(host, port, &hints, &ai);
	if(status != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 2;
	}

	int fd;
	ERR_EXIT(fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));

	if(g_is_server)
	{
		ERR_EXIT(bind(fd, ai->ai_addr, ai->ai_addrlen));
		if(g_is_tcp)
			server_tcp(fd, ai);
		else
			server_udp(fd, ai);
	}
	else
	{
		if(g_is_tcp)
			client_tcp(fd, ai);
		else
			client_udp(fd, ai);
	}
	
	freeaddrinfo(ai);

	return 0;
}
