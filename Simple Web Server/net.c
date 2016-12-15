/* 
 * This is net.c file.
 * Define all network related implementation here.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#include "main.h"

#define DEFAULT_PORT    (8080)

struct Message handle_request(char*);
int Send_Response(int, struct Message);
char * get_current_timestamp(char *, size_t);
int handle_response(int, struct Message);
void set_message_entity(struct Message);
void get_uri_path(char *, struct Message);

int Listen() {
	int serverFd = -1, clientFd = -1;
	struct sockaddr_in6 serveraddr, clientaddr;
	socklen_t addrlen = sizeof(clientaddr);
	char str[INET6_ADDRSTRLEN];
	int LISTENQ = 1024;
	int on = 1;
	struct tm *lt;
	struct timeval to;
	fd_set ready;
	char buf[1024];
	time_t t = time(NULL);
	int rval;
	struct Message m;

	if ((serverFd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("socket() failed");
		exit(0);
	}

	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on))
			< 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
		exit(0);
	}

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin6_family = AF_INET6;

	if (port)
		serveraddr.sin6_port = htons(port);
	else
		serveraddr.sin6_port = htons(DEFAULT_PORT);

	if (ipAddress)
		inet_pton(AF_INET6, ipAddress, &(serveraddr.sin6_addr));
	else
		serveraddr.sin6_addr = in6addr_any;

	if (bind(serverFd, (struct sockaddr *) &serveraddr, sizeof(serveraddr))
			< 0) {
		perror("bind() failed");
		exit(0);
	}

	/* With -d option, we have to accept only one connection */
	if (debugMode)
		LISTENQ = 1;
	else
		LISTENQ = 1024;

	if (listen(serverFd, LISTENQ) < 0) {
		perror("listen() failed");
		exit(0);
	}

	do {
		FD_ZERO(&ready);
		FD_SET(serverFd, &ready);
		to.tv_sec = 5;
		to.tv_usec = 0;
		if (select(serverFd + 1, &ready, 0, 0, &to) < 0) {
			perror("select");
			continue;
		}
		if (FD_ISSET(serverFd, &ready)) {

			/* accept the connection from client */
			if ((clientFd = accept(serverFd, (struct sockaddr *) &clientaddr,
					&addrlen)) < 0) {
				perror("accept() failed");
				break;
			} else {
				getpeername(clientFd, (struct sockaddr *) &clientaddr,
						&addrlen);

				if (inet_ntop(AF_INET6, &clientaddr.sin6_addr, str,
						sizeof(str))) {
					printf("Client address is %s\n", str);
					printf("Client port is %d\n", ntohs(clientaddr.sin6_port));
				}
			}

			/* fortk the new child and close the listening sock*/
			if (fork() == 0) {
				do {
					bzero(buf, sizeof(buf));
					if ((rval = read(clientFd, buf, sizeof(buf))) < 0)
						perror(" reading stream message ");
					else if (rval == 0)
						printf("Ending Connection");

					lt = localtime(&t);
					char *reqtxt = &buf[0];
					printf("flag1\n");
					m = handle_request(reqtxt);
					printf("flag2\n");
					set_message_entity(m);
					handle_response(clientFd, m);

					if (logFile || debugMode) {

						char *pos;

						if ((pos = strchr(reqtxt, '\n')) != NULL)
							*pos = '\0';
						char *now = asctime(lt);

						if ((pos = strchr(now, '\n')) != NULL)
							*pos = '\0';
						sprintf(buf, "%s  %s  %s  %d  %d\n", str, now, &buf[0],
								m.res.status_code, m.res.entity.content_length);

						if (debugMode) {

							if ((pos = strchr(now, '\n')) != NULL)
								*pos = '\0';
							sprintf(buf, "%s  %s  %s  %d  %d\n", str, now,
									&buf[0], m.res.status_code,
									m.res.entity.content_length);

							if (debugMode) {

								fputs(&buf[0], stdout);
							} else {
								FILE *file = fopen(logFile, "ab+");
								fputs(&buf[0], file);
								fclose(file);
							}
						}
					}
				} while (rval > 0);
			}
			close(clientFd);
		}
	} while (1);

	close(serverFd);
	return 1;
}

/*
 * Handle the initial request of client
 */
struct Message handle_request(char* buffer) {

	struct Message m;
	char *endptr;
	int len;

	if (!strncmp(buffer, "GET ", 4)) {
		m.req.method = GET;
		buffer += 4;
	} else if (!strncmp(buffer, "HEAD ", 5)) {
		m.req.method = HEAD;
		buffer += 5;
	} else {
		m.req.method = UNSUPPORTED;
		m.res.status_code = 501;
	}

	/*  Skip to start of resource  */
	while (*buffer && isspace(*buffer))
		buffer++;

	/*  Calculate string length of resource...  */
	endptr = strchr(buffer, ' ');
	if (endptr == NULL)
		len = strlen(buffer);
	else
		len = endptr - buffer;

	if (len == 0)
		m.res.status_code = 400;

	printf("flag4: %s\n", buffer);
	if (strstr(buffer, "HTTP/1.0"))
		m.res.code_reason = "SUPPORTED HTTP VERSION";
	else
		m.res.code_reason = "UNSUPPORTED HTTP VERSION";

	printf("flag3: %s\n", buffer);
	/* Get the URI followed by GET Http method and store it into structure */
	get_uri_path(buffer, m);

	return m;
}

int handle_response(int clientFd, struct Message m) {

	/*  Service the HTTP request  */
	if (m.res.status_code == 200) {
		Send_Response(clientFd, m);
	} else
		return -1;

	return 1;
}

int Send_Response(int clientFd, struct Message m) {
	char buffer[2048];

	char time_str[999];
	if (m.type == RESPONSE) {

		sprintf(buffer, "HTTP 1.0 %d %s\r\n", m.res.status_code,
				m.res.code_reason);
		write(clientFd, buffer, strlen(buffer));

		sprintf(buffer, "Date: %s\r\n",
				get_current_timestamp(time_str, sizeof(time_str)));
		write(clientFd, buffer, strlen(buffer));

		sprintf(buffer, "Server: Simple Web Server 1.0\r\n");
		write(clientFd, buffer, strlen(buffer));

		sprintf(buffer, "Last-Modified: %s\r\n",
				m.res.entity.last_modified_date);
		write(clientFd, buffer, strlen(buffer));

		if (m.res.entity.isCGI == 0) {
			sprintf(buffer, "Content-Type: text/html\r\n");
			write(clientFd, buffer, strlen(buffer));
		}
		if (m.res.entity.isCGI == 0) {
			sprintf(buffer, "Content-Length: %d\r\n",
					m.res.entity.content_length);
			write(clientFd, buffer, strlen(buffer));
		}
		if(m.res.entity.isCGI==0)
			write(clientFd, "\r\n", 2);
	}
	return 1;
}

/*
 * Get current time stamp of server
 */
char * get_current_timestamp(char * time_str, size_t time_str_len) {
	struct tm *tm;
	time_t now;
	now = time(NULL);
	tm = gmtime(&now);
	strftime(time_str, time_str_len, "%a, %d %b %Y %H:%M:%S GMT", tm);
	return time_str;
}

void get_uri_path(char *uri, struct Message m) {
	char _uri[1024];
	char _file[1024];

	memset(_uri, 0x0, sizeof(_uri));
	memset(_file, 0x0, sizeof(_file));

	/*
	 * Skip first '/' from URI
	 */
	strcpy(_uri, &uri[1]);

	/*
	 * IF URI is not started with '~'
	 */
	if (_uri[0] != '~') {

		/* if uri begins with cgi-bin, it means this request is a cgi request */
		if (strncmp(_uri, "cgi-bin", 7) == 0) {

			/* if -c existed */
			if (cgiEnable) {

				/* combine cgi-dir and uri substract cgi-bin */
				strcpy(_file, exCgiDirectory);
				_file[strlen(_file)] = '/';
				strcat(_file, &_uri[7]);

			}
			/* else combine home directory and uri */
			else {
				strcpy(_file, homeDirectory);
				_file[strlen(_file)] = '/';
				strcat(_file, _uri);
			}
		}
		/* else it means it is a regular file request */
		else {
			/* combine home directory and uri */
			strcpy(_file, homeDirectory);
			_file[strlen(_file)] = '/';
			strcat(_file, _uri);
		}
	}
	/* else uri starts with '~' */
	else {

		/*
		 * if the request begins with a '~', then the following string
		 * up to the first slash is translated into that user's sws
		 * directory (ie /home/<user>/sws/).
		 */

		printf("first URI: %s", _uri);
		char * p = _uri + 1; /* point to first char of user */
		printf("second URI: %s", p);
		char user[1024];
		memset(user, 0x0, sizeof(user));
		char * q = user;
		char c;

		while (((c = (*p)) != '/') && ((c = (*p)) != '\0')
				&& ((c = (*p)) != ' ')) {
			*q = c;
			p++;
			q++;
		}
		*(q + 1) = '\0';

		printf("user=%s  q=%s file=%s\n", user, q, _file);
		/* construct file path */
		strcat(_file, "/home/");
		strcat(_file, user);
		strcat(_file, "/sws/");
		printf("user=%s  q=%s file=%s\n", user, q, _file);
		strcat(_file, p);
	}
	strcpy(m.req.URI, _file);
}


void set_message_entity(struct Message m) {
	FILE *resource;
	char *content;
	int numbytes;

	if ((resource = fopen(m.req.URI, "r")) == NULL) {
		m.res.status_code = 404;
	} else {
		fseek(resource, 0L, SEEK_END);
		numbytes = ftell(resource);
		fseek(resource, 0L, SEEK_SET);
		content = (char*) calloc(numbytes, sizeof(char));
		fread(content, sizeof(char), numbytes, resource);
		m.res.entity.content = content;
		m.res.entity.content_length = numbytes;
		m.res.entity.content_type = "text/html";
	}
	printf("%s", content);
	fclose(resource);
}
