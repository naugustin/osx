/*	$OpenBSD: socks.c,v 1.15 2005/05/24 20:13:28 avsm Exp $	*/

/*
 * Copyright (c) 1999 Niklas Hallqvist.  All rights reserved.
 * Copyright (c) 2004, 2005 Damien Miller.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "atomicio.h"

#define SOCKS_PORT	"1080"
#define HTTP_PROXY_PORT	"3128"
#define HTTP_MAXHDRS	64
#define SOCKS_V5	5
#define SOCKS_V4	4
#define SOCKS_NOAUTH	0
#define SOCKS_NOMETHOD	0xff
#define SOCKS_CONNECT	1
#define SOCKS_IPV4	1
#define SOCKS_DOMAIN	3
#define SOCKS_IPV6	4

int	remote_connect(const char *, const char *, struct addrinfo);
int	socks_connect(const char *host, const char *port, struct addrinfo hints,
	    const char *proxyhost, const char *proxyport, struct addrinfo proxyhints,
	    int socksv);

static int
decode_addrport(const char *h, const char *p, struct sockaddr *addr,
    socklen_t addrlen, int v4only, int numeric)
{
	int r;
	struct addrinfo hints, *res;

	bzero(&hints, sizeof(hints));
	hints.ai_family = v4only ? PF_INET : PF_UNSPEC;
	hints.ai_flags = numeric ? AI_NUMERICHOST : 0;
	hints.ai_socktype = SOCK_STREAM;
	r = getaddrinfo(h, p, &hints, &res);
	/* Don't fatal when attempting to convert a numeric address */
	if (r != 0) {
		if (!numeric) {
			errx(1, "getaddrinfo(\"%.64s\", \"%.64s\"): %s", h, p,
			    gai_strerror(r));
		}
		return (-1);
	}
	if (addrlen < res->ai_addrlen) {
		freeaddrinfo(res);
		errx(1, "internal error: addrlen < res->ai_addrlen");
	}
	memcpy(addr, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	return (0);
}

static size_t
proxy_read_line(int fd, char *buf, size_t bufsz)
{
	size_t off;

	for(off = 0;;) {
		if (off >= bufsz)
			errx(1, "proxy read too long");
		if (atomicio(read, fd, buf + off, 1) != 1)
			err(1, "proxy read");
		/* Skip CR */
		if (buf[off] == '\r')
			continue;
		if (buf[off] == '\n') {
			buf[off] = '\0';
			break;
		}
		off++;
	}
	return (off);
}

int
socks_connect(const char *host, const char *port,
    struct addrinfo hints __attribute__ ((__unused__)),
    const char *proxyhost, const char *proxyport, struct addrinfo proxyhints,
    int socksv)
{
	int proxyfd;
	size_t r;
	size_t hlen, wlen;
	unsigned char buf[1024];
	size_t cnt;
	struct sockaddr_storage addr;
	struct sockaddr_in *in4 = (struct sockaddr_in *)&addr;
	struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&addr;
	in_port_t serverport;

	if (proxyport == NULL)
		proxyport = (socksv == -1) ? HTTP_PROXY_PORT : SOCKS_PORT;

	proxyfd = remote_connect(proxyhost, proxyport, proxyhints);

	if (proxyfd < 0)
		return (-1);

	/* Abuse API to lookup port */
	if (decode_addrport("0.0.0.0", port, (struct sockaddr *)&addr,
	    sizeof(addr), 1, 1) == -1)
		errx(1, "unknown port \"%.64s\"", port);
	serverport = in4->sin_port;

	if (socksv == 5) {
		if (decode_addrport(host, port, (struct sockaddr *)&addr,
		    sizeof(addr), 0, 1) == -1)
			addr.ss_family = 0; /* used in switch below */

		/* Version 5, one method: no authentication */
		buf[0] = SOCKS_V5;
		buf[1] = 1;
		buf[2] = SOCKS_NOAUTH;
		cnt = atomicio(vwrite, proxyfd, buf, 3);
		if (cnt != 3)
			err(1, "write failed (%lu/3)", cnt);

		cnt = atomicio(read, proxyfd, buf, 2);
		if (cnt != 2)
			err(1, "read failed (%lu/3)", cnt);

		if (buf[1] == SOCKS_NOMETHOD)
			errx(1, "authentication method negotiation failed");

		switch (addr.ss_family) {
		case 0:
			/* Version 5, connect: domain name */

			/* Max domain name length is 255 bytes */
			hlen = strlen(host);
			if (hlen > 255)
				errx(1, "host name too long for SOCKS5");
			buf[0] = SOCKS_V5;
			buf[1] = SOCKS_CONNECT;
			buf[2] = 0;
			buf[3] = SOCKS_DOMAIN;
			buf[4] = hlen;
			memcpy(buf + 5, host, hlen);			
			memcpy(buf + 5 + hlen, &serverport, sizeof serverport);
			wlen = 7 + hlen;
			break;
		case AF_INET:
			/* Version 5, connect: IPv4 address */
			buf[0] = SOCKS_V5;
			buf[1] = SOCKS_CONNECT;
			buf[2] = 0;
			buf[3] = SOCKS_IPV4;
			memcpy(buf + 4, &in4->sin_addr, sizeof in4->sin_addr);
			memcpy(buf + 8, &in4->sin_port, sizeof in4->sin_port);
			wlen = 10;
			break;
		case AF_INET6:
			/* Version 5, connect: IPv6 address */
			buf[0] = SOCKS_V5;
			buf[1] = SOCKS_CONNECT;
			buf[2] = 0;
			buf[3] = SOCKS_IPV6;
			memcpy(buf + 4, &in6->sin6_addr, sizeof in6->sin6_addr);
			memcpy(buf + 20, &in6->sin6_port,
			    sizeof in6->sin6_port);
			wlen = 22;
			break;
		default:
			errx(1, "internal error: silly AF");
		}

		cnt = atomicio(vwrite, proxyfd, buf, wlen);
		if (cnt != wlen)
			err(1, "write failed (%lu/%lu)", cnt, wlen);

		cnt = atomicio(read, proxyfd, buf, 10);
		if (cnt != 10)
			err(1, "read failed (%lu/10)", cnt);
		if (buf[1] != 0)
			errx(1, "connection failed, SOCKS error %d", buf[1]);
	} else if (socksv == 4) {
		/* This will exit on lookup failure */
		decode_addrport(host, port, (struct sockaddr *)&addr,
		    sizeof(addr), 1, 0);

		/* Version 4 */
		buf[0] = SOCKS_V4;
		buf[1] = SOCKS_CONNECT;	/* connect */
		memcpy(buf + 2, &in4->sin_port, sizeof in4->sin_port);
		memcpy(buf + 4, &in4->sin_addr, sizeof in4->sin_addr);
		buf[8] = 0;	/* empty username */
		wlen = 9;

		cnt = atomicio(vwrite, proxyfd, buf, wlen);
		if (cnt != wlen)
			err(1, "write failed (%lu/%lu)", cnt, wlen);

		cnt = atomicio(read, proxyfd, buf, 8);
		if (cnt != 8)
			err(1, "read failed (%lu/8)", cnt);
		if (buf[1] != 90)
			errx(1, "connection failed, SOCKS error %d", buf[1]);
	} else if (socksv == -1) {
		/* HTTP proxy CONNECT */

		/* Disallow bad chars in hostname */
		if (strcspn(host, "\r\n\t []:") != strlen(host))
			errx(1, "Invalid hostname");

		/* Try to be sane about numeric IPv6 addresses */
		if (strchr(host, ':') != NULL) {
			r = snprintf((char *)buf, sizeof(buf),
			    "CONNECT [%s]:%d HTTP/1.0\r\n\r\n",
			    host, ntohs(serverport));
		} else {
			r = snprintf((char *)buf, sizeof(buf),
			    "CONNECT %s:%d HTTP/1.0\r\n\r\n",
			    host, ntohs(serverport));
		}
		if (r == -1 || (size_t)r >= sizeof(buf))
			errx(1, "hostname too long");
		r = strlen((char *)buf);

		cnt = atomicio(vwrite, proxyfd, buf, r);
		if (cnt != r)
			err(1, "write failed (%lu/%zd)", cnt, r);

		/* Read reply */
		for (r = 0; r < HTTP_MAXHDRS; r++) {
			proxy_read_line(proxyfd, (char *)buf, sizeof(buf));
			if (r == 0 && strncmp((char *)buf, "HTTP/1.0 200 ", 12) != 0)
				errx(1, "Proxy error: \"%s\"", buf);
			/* Discard headers until we hit an empty line */
			if (*buf == '\0')
				break;
		}
	} else
		errx(1, "Unknown proxy protocol %d", socksv);

	return (proxyfd);
}
