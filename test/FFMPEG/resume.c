#include <stdio.h>
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>
//int backtrace(void **buffer,int size)
//char ** backtrace_symbols (void *const *buffer, int size)
//void backtrace_symbols_fd (void *const *buffer, int size, int fd)  

void saveTrace()
{
  void *array[10];

  int size = backtrace(array, 10);

  int fd = open("/tmp/trace.info", O_RDWR | O_CREAT);
  backtrace_symbols_fd(array, size, stdout);

  close(fd);
}

void printTrace()
{
  void *array[10];
  char **strs;

  int size = backtrace(array, 10);
  strs = backtrace_symbols(array, size);

  int i;
  for (i = 0; i < size; i++)
  {
    printf ("%s\n", strs[i]);
  }

  free (strs);
  strs = NULL;
}

void getMemory(char **p)
{
  *p = (char *)malloc(100 * sizeof (char));

  printTrace();
  saveTrace();
}

int main(int argc, char const *argv[])
{
  char *p = NULL;
  getMemory(&p);

  strcpy(p, "hello world.");

  fprintf(stdout, "%s\n", p);

  return 0;
}