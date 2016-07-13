#include "http.h"
/* Set the request named NAME to VALUE. */
static char request_string[MAXURL + 4096];
static char host[MAXURL];
static char arg[MAXURL];

static void response_free(struct resp_headers *resp)
{
    free(resp->data);
    free(resp->headers);
    free(resp);
}

static int request_set_header(struct request *req, const char *name, char *value)
{
    struct request_header *hdr, *np;
    int i;

    for (i = 0; i < req->hcount; i++) {          // replace exist header
        hdr = &req->headers[i];
        if (strcasecmp(hdr->name, name) == 0) {
            hdr->value = value;
            return 0;
        }
    }

    if (req->hcount >= req->hlimit) {
        req->hlimit *= 2;
        np = realloc(req->headers, req->hlimit * sizeof(*hdr));
        if (np == NULL) {
            fprintf(stderr, "request_set_header: realloc error\n");
            return -1;
        }

        req->headers = np;
    }

    hdr = &req->headers[req->hcount++];
    hdr->name = name;
    hdr->value = value;
    return 0;
}

/* Create a new, empty request. Set the request's method and its
   arguments. */
static struct request *request_new(const char *method, char *arg)
{
    struct request *req;

    if ((req = malloc(sizeof(struct request))) == NULL) {
        fprintf(stderr, "debug: request_new, malloc error\n");
        return NULL;
    }

    req->hlimit = 8;
    req->hcount = 0;
    req->method = method;
    req->arg = arg;
    req->headers = calloc(req->hlimit, sizeof(struct request_header));

    return req;
}

/* parse url and save to global string buffer host & arg * */
void parse_url(const char *url)
{
    char *p;

    p = strstr(url, ":/");
    if (p == NULL) {
        strcpy(host, url);
    } else {
        p += 3;  // skip ://
        strcpy(host, p);
    }

    p = strchr(host, '/');

    if (p == NULL) {
        strcpy(arg, "/");
    } else {
        strcpy(arg, p);
        *p = '\0';  // end host
    }
}

static size_t create_req_str(struct request *req)
{
    struct request_header *hdr;
    char *p;
    size_t size = 0;
    int i;

    /* GET /urlpath HTTP/1.0 \r\n */
    size += strlen(req->method) + 1;
    size += strlen(req->arg) + 1 + 8 + 1 + 2;
    for (i = 0; i < req->hcount; i++) {
        /* header_name: header_value\r\n */
        hdr = &req->headers[i];
        size += strlen(hdr->name) + 2 + strlen(hdr->value) + 2;
    }

    size += 2;   // "\r\n"

    // add 1 for '\0', otherwise valgrind show Invalid write of size 1
    p = request_string;
    memset(request_string, 0, sizeof(request_string));
    strcat(p, req->method), strcat(p, " ");
    strcat(p, req->arg);
    strcat(p, " HTTP/1.1\r\n");
    for (i = 0; i < req->hcount; i++) {
        hdr = &req->headers[i];
        strcat(p, hdr->name), strcat(p, ": ");
        strcat(p, hdr->value), strcat(p, "\r\n");
    }
    const char *accept = "Accept: */*\r\n";
    const char *user_agent = "User-Agent: (null)\r\n";
    const char *range = "Range: bytes=0-\r\n";
    const char *icy = "Icy-MetaData: 1\r\n";
    const char *closetag = "Connection: close\r\n";
    strcat(p, accept);
    strcat(p, user_agent);
    strcat(p, range);
    strcat(p, closetag);
    strcat(p, icy);
    strcat(p, "\r\n");
    return size + strlen(accept) + strlen(user_agent) + strlen(range) + strlen(closetag) + strlen(icy);
}

/* Release the resources used by REQ. */
static void request_free (struct request *req)
{
    if (req->headers)
        free(req->headers);
    free(req);
}

/* open connect and send request, return a socket fd */
struct request *request_send(char *method, const char *url)
{
    struct request *req;
    size_t size;

    int sockfd;
    parse_url(url);

    if ((sockfd = tcp_connect(host, 80)) < 0) {
        fprintf(stderr, "http_get: can not create connection to %s\n", host);
        return NULL;
    }

    req = request_new(method, arg);
    req->sockfd = sockfd;
    request_set_header(req, "Host", host);
    //request_set_header(req, "Accept-Encoding", "gzip");
    //request_set_header(req, "User-Agent", DEFAULT_USER_AGENT);

    size = create_req_str(req);
    //fprintf(stdout, "request_string=%s\n", request_string);
    if (writen(sockfd, request_string, size) == -1)
    {
        fprintf(stderr, "http_get: fail send request to %s\n", host);
        request_free(req);
        return NULL;
    }

    //request_free(req);
    return req;
}

/* parse response header, return http status code , -1 on error */
static int parse_header(struct resp_headers *resp, char *head)
{
    char **np;
    char *p, *s;
    char status_buf[STATUS_BUF];
    int status;

    resp->data = head;
    resp->hcount = 0;
    resp->hlimit = 16;
    resp->headers = malloc(resp->hlimit * sizeof(char **));
    memset(resp->headers, 0, resp->hlimit * sizeof(char **));

    resp->headers[resp->hcount++] = head;
    // header end with \r\n\0, the \r in the extra \r\n is overwritten by \0
    p = head;
    for ( ; p[2] != '\0'; p++) {
        if (p[0] != '\r' && p[1] != '\n')
            continue;

        if (resp->hcount >= resp->hlimit) {
            resp->hlimit *= 2;
            np = realloc(resp->headers, resp->hlimit * sizeof(char **));
            if (np == NULL) {
                fprintf(stderr, "parse_header: realloc error\n");
                free(resp->data);
                free(resp->headers);
                return -1;
            }

            resp->headers = np;
        }

        *(p + 1) = 0;
        resp->headers[resp->hcount++] = p + 2;
    }

    strncpy(status_buf, head, STATUS_BUF);
    s = strchr(status_buf, ' ');
    s++;
    p = strchr(s, ' ');
    *p = '\0';
    status = atoi(s);

    return status;
}


char *read_drain_headers(int fd)
{
    char *buf, *p;
    ssize_t npeek, nread;
    size_t i, headlen;
    size_t buflen = PEEK_SIZE;

    buf = NULL;
    while (buflen < MAX_HEADER_SIZE) {
        if (buf != NULL)
            free(buf);

        if ((buf = calloc(1, buflen)) == NULL) {
            fprintf(stderr, "read_drain_headers: calloc error\n");
            return NULL;
        }

        npeek = recv(fd, buf, buflen, MSG_PEEK);
        if (npeek == -1 && errno == EINTR)
            continue;

        /* looking for header terminator \n\r\n */
        p = buf;
        for (i = 0; i < (buflen - 2); i++) {
            if (p[i] == '\n' && p[i + 1] == '\r' && p[i + 2] == '\n') {
                /* read & drain header from sockfd */
                headlen = i + 3;
                if ((nread = readn(fd, buf, headlen)) == -1) {
                    free(buf);
                    return NULL;
                }

                //assert(headlen == nread);
                buf[i + 1] = '\0';

                return buf;
            }
        }

        /* hasn't find terminator, double peek size and try again */
        buflen *= 2;
    }

    fprintf(stderr, "read_drain_headers: header too big\n");
    return NULL;
}

/* get a malloc copy of header value, NULL if not found */
static char *response_head_strdup(struct resp_headers *resp, char *name)
{
    const char *p;
    const char *next;
    char *h, *value;
    int i;
    size_t buflen;
    size_t namelen = strlen(name);

    p = NULL;
    for (i = 0; i < resp->hcount; i++) {
        if (strncasecmp(name, resp->headers[i], namelen) == 0) {
            p = resp->headers[i];
            break;
        }
    }

    if (p == NULL)
        return NULL;

    if (i == (resp->hcount - 1))
        next = resp->headers[0] + strlen(resp->data);
    else
        next = resp->headers[i + 1]; // - resp->headers[0];

    h = strchr(p, ':'), h++;   // skip :
    while (isspace(*h)) {
        h++;
    }

    buflen = next - h - 2 + 1;   // minus \r\n, plus '\0'

    fprintf(stdout, "response_head_strdup=%lu\n", buflen);
    
    value = calloc(1, buflen);
    strncpy(value, h, buflen);
    value[buflen - 1] = '\0';

    return value;
}

int parseHttpRespond(int fd, FILE *fp)
{
    int status;
    struct resp_headers *header;
    char *recv_buf = read_drain_headers(fd);
    char *red_url;
    //fprintf(stdout, "recv_buf=%s\n", recv_buf);

    header = calloc(1, sizeof(struct resp_headers));
    if ((status = parse_header(header, recv_buf)) == -1) {
        fprintf(stderr, "http_get %s%s: fail to parse response header\n",
                host, arg);
        return -1;
    }

    fprintf(stdout, "parse header. status=%d\n", status);

    if (status >= 403)
    {
        return -1;
    }
    // int red_count = 0;
    //  while (300 < status && status < 400 && red_count++ < REDIRECT_LIMIT) {
    //     printf("HTTP %d: redirect to %s%s\n", status, host, arg);
    //     red_url = response_head_strdup(header, "Location");
        
    //     fprintf(stdout, "redirect red_url=%s\n", red_url);

    //     response_free(header);
    //     if (red_url[0] == '/')
    //         strncpy(arg, red_url, MAXURL);
    //     else
    //         parse_url(red_url);

    //     fprintf(stdout, "redirect arg=%s host=%s \n", arg, host);
    //     free(red_url);
    //     close(fd);
    //     if ((fd = request_send("GET", host)) == -1) {
    //         fprintf(stderr, "http_get %s: fail send request\n", host);
    //         return -1;
    //     }

    //     if ((recv_buf = read_drain_headers(fd)) == NULL) {
    //         fprintf(stderr, "http_get %s: fail to read response header\n", host);
    //         return -1;
    //     }

    //     header = calloc(1, sizeof(struct resp_headers));
    //     if ((status = parse_header(header, recv_buf)) == -1) {
    //         fprintf(stderr, "http_get %s%s: fail to parse response header\n",
    //                 host, arg);
    //         return -1;
    //     }
    // }

    uint64_t content_len = 0;
    int type;
    int i;
    for (i = 0; i < header->hcount; ++i)
    {
        if (strstr(header->headers[i], "Content-Length") != NULL)
        {
            fprintf(stdout, "header:%d %s\n", i, header->headers[i]);
            
            sscanf(header->headers[i], "%*s%lu", &content_len);
        }
    }
    int readSize = parseMp4Box(fd, &type);

    fprintf(stdout, "parseMp4Box %lu %lu %d\n", content_len, (type == 0) ? readSize : (content_len - readSize), type);
    fprintf(fp, "%lu\t%lu\t%d\n", content_len, (type == 0) ? readSize : content_len - readSize, type);
}


void http_task(const char *url, FILE *output)
{
    struct request *req = request_send("GET", url);
    if (req != NULL)
    {
        //CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        parseHttpRespond(req->sockfd, output);

        close(req->sockfd);
        request_free(req);
    }
}




