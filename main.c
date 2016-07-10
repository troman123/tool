#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <errno.h>

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

int main(int argc, char const *argv[])
{
  //assert(argc >= 2);

  char hostname[1024], hoststr[1024], proto[10];
  char auth[1024], proxyauth[1024] = "";
  char path[MAX_URL_SIZE];
  char buf[1024], urlbuf[MAX_URL_SIZE];
  int port, use_proxy, err, location_changed = 0, redirects = 0, attempts = 0;

  const char *url = "http://record.vod.huanjuyun.com/xcrs/15013x03_1330846705_1554444831_1467210406_1467210388.m3u8";
  av_url_split(proto, sizeof(proto),
                auth, sizeof(auth),
                hostname, sizeof(hostname),
                &port,
                path, sizeof(path),
                url);

  fprintf(stdout, "parse url:proto=%s \nauth=%s \nhostname=%s \nport=%d \npath=%s\n",
   proto, auth, hostname, port, path);

  if (port < 0)
  {
    port = 80;
  }
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  assert( sock_fd >= 0 );

  struct hostent* hostinfo = gethostbyname(hostname);
  assert(hostinfo);
  //char *ip;
  struct sockaddr_in server;
  bzero( &server, sizeof( server ) );
  server.sin_family = AF_INET;
  server.sin_addr = *((struct in_addr*)*(hostinfo->h_addr_list));
  //inet_pton( AF_INET, ip, &server.sin_addr );
  server.sin_port = htons( port );

  int connfd = connect(sock_fd, (struct sockaddr*)&server,  sizeof(server));
  assert(connfd != -1);
  // int ret = bind( sock_fd, ( struct sockaddr* )&server, sizeof( server ) );
  // assert(ret != 0);

  char buffer[ BUFFER_SIZE ];
  memset( buffer, '\0', BUFFER_SIZE );
  int data_read = 0;
  int read_index = 0;
  int checked_index = 0;
  int start_line = 0;
  //CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
  while( 1 )
  {
      data_read = recv( connfd, buffer + read_index, BUFFER_SIZE - read_index, 0 );
      fprintf(stdout, "recv %d\n", data_read);
      if ( data_read == -1 )
      {
          printf( "reading failed %s\n" , strerror(errno));
          break;
      }
      else if ( data_read == 0 )
      {
          printf( "remote client has closed the connection\n" );
          break;
      }


      // read_index += data_read;
      // HTTP_CODE result = parse_content( buffer, checked_index, checkstate, read_index, start_line );
      // if( result == NO_REQUEST )
      // {
      //     continue;
      // }
      // else if( result == GET_REQUEST )
      // {
      //     send( connfd, szret[0], strlen( szret[0] ), 0 );
      //     break;
      // }
      // else
      // {
      //     send( connfd, szret[1], strlen( szret[1] ), 0 );
      //     break;
      // }
  }
  close( connfd );
  close(sock_fd);

  return 0;
}