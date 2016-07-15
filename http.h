//#include "mp4parser.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>


#define DEFAULT_USER_AGENT "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.13) Gecko"
#define MAXURL 2048  /* IE's limit is around 204x */
#define PEEK_SIZE 1024
#define MAX_HEADER_SIZE 4 * 1024
#define HTTP_BUFSIZE 16 * 1024
#define CHUNK 16 * 1024  /* zlib inflate buf chunk, can be as small as 1 */
#define STATUS_BUF 32
#define REDIRECT_LIMIT 5

struct request {
    int sockfd;
    const char *method;

    char *arg;
    struct request_header {
        const char *name;
        char *value;
    } *headers;

    int hcount;
    int hlimit;
};

struct resp_headers {
    char *data;

/* From wget: The array of pointers that indicate where each header starts.
For example, given this HTTP response:

HTTP/1.0 200 Ok
Description: some
text
Etag: x

The headers are located like this:

"HTTP/1.0 200 Ok\r\nDescription: some text\r\nEtag: some text\r\n\r\n"
^                   ^                         ^                  ^
headers[0]          headers[1]                headers[2]         \0 */
    char **headers;
    int hcount;
    int hlimit;
};


typedef struct http_sys_t
{
    char request_string[MAXURL + 4096];
    char host[MAXURL];
    char arg[MAXURL];

    struct request *req;
    struct resp_headers *resp;
} http_sys_t;