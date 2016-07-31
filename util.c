#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <errno.h>

/* from UNP */
ssize_t writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            fprintf(stderr, "write nwritten=%ld nleft=%lu\n", nwritten, nleft);
            if (errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }

    return n;
}

ssize_t readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            fprintf(stderr, "read nread=%ld nleft=%lu\n", nread, nleft);
            if (errno == EINTR)
                nread = 0;
            else
                return -1;
        } else if (nread == 0)      // EOF
            break;

        nleft -= nread;
        bufp += nread;
    }

    return (n - nleft);
}

#define BUFFER_SIZE 4096
#define FFMIN(a, b) (a < b) ? a : b
#define MAX_URL_SIZE 4096

size_t av_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = 0;
    while (++len < size && *src)
        *dst++ = *src++;
    if (len <= size)
        *dst = 0;
    return len + strlen(src) - 1;
}

void av_url_split(char *proto, int proto_size,
                  char *authorization, int authorization_size,
                  char *hostname, int hostname_size,
                  int *port_ptr,
                  char *path, int path_size,
                  const char *url)
{
    const char *p, *ls, *ls2, *at, *at2, *col, *brk;

    if (port_ptr)               *port_ptr = -1;
    if (proto_size > 0)         proto[0] = 0;
    if (authorization_size > 0) authorization[0] = 0;
    if (hostname_size > 0)      hostname[0] = 0;
    if (path_size > 0)          path[0] = 0;

    /* parse protocol */
    if ((p = strchr(url, ':'))) {
        av_strlcpy(proto, url, FFMIN(proto_size, p + 1 - url));
        p++; /* skip ':' */
        if (*p == '/') p++;
        if (*p == '/') p++;
    } else {
        /* no protocol means plain filename */
        av_strlcpy(path, url, path_size);
        return;
    }

    /* separate path from hostname */
    ls = strchr(p, '/');
    ls2 = strchr(p, '?');
    if(!ls)
        ls = ls2;
    else if (ls && ls2)
        ls = FFMIN(ls, ls2);
    if(ls)
        av_strlcpy(path, ls, path_size);
    else
        ls = &p[strlen(p)]; // XXX

    /* the rest is hostname, use that to parse auth/port */
    if (ls != p) {
        /* authorization (user[:pass]@hostname) */
        at2 = p;
        while ((at = strchr(p, '@')) && at < ls) {
            av_strlcpy(authorization, at2,
                       FFMIN(authorization_size, at + 1 - at2));
            p = at + 1; /* skip '@' */
        }

        if (*p == '[' && (brk = strchr(p, ']')) && brk < ls) {
            /* [host]:port */
            av_strlcpy(hostname, p + 1,
                       FFMIN(hostname_size, brk - p));
            if (brk[1] == ':' && port_ptr)
                *port_ptr = atoi(brk + 2);
        } else if ((col = strchr(p, ':')) && col < ls) {
            av_strlcpy(hostname, p,
                       FFMIN(col + 1 - p, hostname_size));
            if (port_ptr) *port_ptr = atoi(col + 1);
        } else
            av_strlcpy(hostname, p,
                       FFMIN(ls + 1 - p, hostname_size));
    }
}

void printHostIp(const struct hostent * host)
{
  char str[32];
  char **pptr = NULL;
  for (pptr = host->h_aliases; *pptr != NULL; pptr++)
  {
    printf(" alias:%s\n",*pptr);
  }

  switch(host->h_addrtype)
  {
    case AF_INET:
    //case AF_INET6:
      pptr = host->h_addr_list;
      for(; *pptr != NULL; pptr++)
      {
        printf(" address:%s\n", inet_ntop(host->h_addrtype, *pptr, str, sizeof(str)));
      }

      printf(" first address: %s\n", inet_ntop(host->h_addrtype, host->h_addr, str, sizeof(str)));

      break;
    default:
      printf("unknown address type\n");
      break;
  }
}

int tcp_connect(const char *host, int port)
{
    int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    //assert( sock_fd != -1 );
    if (sock_fd == -1)
    {
      fprintf(stderr, "socket fail. %s\n", strerror(errno));
      return -1;
    }

    struct hostent* hostinfo = gethostbyname(host);
    assert(hostinfo);

    printHostIp(hostinfo);

    struct sockaddr_in server;
    bzero( &server, sizeof( server ) );
    server.sin_family = AF_INET;
    server.sin_addr = *((struct in_addr*)(hostinfo->h_addr));
    //inet_pton( AF_INET, ip, &server.sin_addr );
    server.sin_port = htons( port );

    int ret = connect(sock_fd, (struct sockaddr*)&server,  sizeof(server));
    //assert(ret != -1);
    if (ret == -1)
    {
      fprintf(stderr, "connect fail. %s\n", strerror(errno));
      return -1;
    }

    fprintf(stdout, "tcp conn: host=%s port=%d sock_fd=%d ret=%d\n", host, port, sock_fd, ret);

    return sock_fd;
}

/* connect to host:serv, return socket file descriptor, -1 on error */
int tcp_connect2(const char *host, const char *serv)
{
    int n, sockfd;
    struct addrinfo hints;
    struct addrinfo *res, *rp;

    memset(&hints, 0, sizeof(hints));
    //hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_UNSPEC;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1)
            continue;

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                 /* Success */

        close(sockfd);
    }

    if (rp == NULL) {             /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        return -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

char *base64url_to_base64(const char *data);
char *decode(const char *data)
{
  char *result = NULL;

  char *tmp = base64url_to_base64(data);
  int len = Base64decode_len(tmp);
  char *buf = (char *) malloc(len);
  len = Base64decode(buf, tmp);
  free(tmp);

  return buf;
}

char *base64url_to_base64(const char *data)
{
  size_t len = strlen(data) + 1;
  char *tmp = (char *) malloc(len);
  int i = 0;
  for (i = 0; i < len; i++)
  {
    switch (data[i])
    {
    case '-':
      tmp[i] = '+';
      break;

    case '_':
      tmp[i] = '/';
      break;

    default:
      tmp[i] = data[i];
      break;
    }
  }

  return tmp;
}