#include "threadpool.h"
#include "util.h"

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

#define THREAD 1
#define QUEUE  4

#include "urldecode.h"
static FILE *gfp = NULL;
static int URL_ACCESS_TIMES = 0;
static uint64_t access_sum = 0;

typedef struct thread_sys_t
{
  int type;
  FILE *fp;
  threadpool_t *pool
} thread_sys_t;

pthread_mutex_t lock;
void thread_task(void *arg)
{
  fprintf(stdout, "thread_task run. %lu\n", pthread_self());
  thread_sys_t *threadmgr = (thread_sys_t *)arg;
  char read_line[1024];
  int count = 0x7fffffff;

  pthread_mutex_lock(&lock);
  if ((fgets(read_line, 1000, threadmgr->fp) != NULL))
  {
    pthread_mutex_unlock(&lock);

    fprintf(stdout, "read line read_line=%s\n", read_line);
    sscanf(read_line,"%s\t%d", read_line, &count);
    fprintf(stderr, "read line read_line read_line=%s  count=%d\n", read_line, count);
    if (count >= URL_ACCESS_TIMES)
    {
      access_sum += count;
      char *url_decode = urlDecode(read_line);
      char *tmp = strstr(url_decode, "\"");
      if (tmp != NULL)
      {
        *tmp = 0;
      }
      fprintf(stdout, "read line url_decode=%s count=%d\n", url_decode, count);

      if (strstr(url_decode, "http://") != NULL)
      {
        http_task(url_decode, gfp, threadmgr->type, count);
      }

      free(url_decode);

      pthread_mutex_lock(&lock);
      int tasks = 0;
      while (threadpool_add(threadmgr->pool, &thread_task, threadmgr, 0) == 0)
      {
          tasks++;
      }

      fprintf(stderr, "Added thread %d tasks.\n", tasks);

      pthread_mutex_unlock(&lock);
    }
  }
  else
  {
    //perror("fgets read fail.");
    fprintf(stderr, "fgets read fail.%s\n", strerror(errno));
  }

  fprintf(stdout, "thread_task out. %lu\n", pthread_self());
}


void thread_pool_task(FILE *fp, int type)
{
  fprintf(stdout, "thread pool\n");

  pthread_mutex_init(&lock, NULL);

  thread_sys_t *threadmgr = malloc(sizeof(thread_sys_t));
  threadpool_t *pool;

  assert((pool = threadpool_create(THREAD, QUEUE, 0)) != NULL);

  threadmgr->fp = fp;
  threadmgr->pool = pool;
  threadmgr->type = type;
  fprintf(stdout, "Pool started with %d threads and " "queue size of %d\n", THREAD, QUEUE);

  //char *url = "http://record.vod.huanjuyun.com/xcrs/15013x03_1330846705_1554444831_1467210406_1467210388.m3u8";
  //threadpool_add(pool, &task, url, 0);

  int tasks = 0;
  while (threadpool_add(pool, &thread_task, threadmgr, 0) == 0)
  {
      tasks++;
  }

  fprintf(stderr, "Added thread %d tasks\n", tasks);

  int status = threadpool_destroy(pool, 1);

  free(threadmgr);

  fprintf(stdout, "destroy threadpool status=%d\n", status);
}

void test_readfile()
{
  FILE *fp = NULL;
  fp = fopen("/mnt/hgfs/E/download/mp4_output.txt", "r");
  if (fp != NULL)
  {
    char str[1024];
    int c = 0;
    while (fgets(str, 1024, fp) != NULL && c < 10)
    {
      fprintf(stdout, "read line str=%s\n", str);
      int count = 0;
      sscanf(str, "\"%s\t%d", str, &count);
      fprintf(stdout, "str=%s count=%d\n", str, count);

      c++;
    }
  }
}

void test_sscanf()
{
  const char *str = "Content-Length: 68430";
  int len = 0;
  sscanf(str, "%*s%d", &len);

  fprintf(stdout, "%d\n", len);
}

#define PARSE_TAG "mp4"//"mp4" //"m3u8"

void parse_urls(const char *parsetag)
{
  //
  FILE *fp;
  char file_name[1024];
  if (strstr(parsetag, "m3u8") != NULL)
  {
    remove("output_m3u8.txt");
    if (gfp == NULL)
    {
        gfp = fopen("output_m3u8.txt", "a");
    }
    fp = fopen("/mnt/hgfs/E/download/outfile2.txt", "r");
    if (fp != NULL)
    {
      thread_pool_task(fp, FILE_TYPE_M3U8);
      fclose(fp);
    }
    else
    {
      perror("open file fail.");
    }
    
  }
  else if (strstr(parsetag, "mp4") != NULL)
  {
    remove("output_mp4.txt");
    if (gfp == NULL)
    {
        gfp = fopen("output_mp4.txt", "a");
    }
#define TMP_TEST 1

#ifdef TMP_TEST
    fp = fopen("mp4_zhaoguoqing.txt", "r");
    if (fp != NULL)
    {
      thread_pool_task(fp, FILE_TYPE_MP4);
      fclose(fp);
    }
    else
    {
      perror("open file fail2.");
    }
#else
    int i = 0;
    int index = 1;//16;
    for (i = 0; i <= index; ++i)
    {
      sprintf(file_name, "/mnt/hgfs/E/download/split_csv_file/mp4_urls_%d_outfile.txt", i);
      fprintf(stdout, "open file %s\n", file_name);
      fp = fopen(file_name, "r");
      if (fp != NULL)
      {
        thread_pool_task(fp, FILE_TYPE_MP4);
        fclose(fp);
      }
      else
      {
        perror("open file fail2.");
      }
    }
#endif

  }
  if (gfp != NULL)
  {
    fclose(gfp);
  }
}

void Test_parseurls()
{
  parse_urls(PARSE_TAG);

  fprintf(stdout, "access_sum=%lld\n", access_sum);
}

void Test_args()
{
  valist_args("/moov");
  valist_args("/moov", 1);
}

void Test_Realloc()
{

}

void Test_cost()
{
  multi_read_mutex_cost();
}

int main(int argc, char const *argv[])
{
  Test_parseurls();
  //Test_args();
  //realloc_exam();

  //Test_cost();

  //realloc_exam2();
  return 0;
}