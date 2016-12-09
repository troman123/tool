#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef uint32_t UINT32;

void fun3(void)
{
  void* array[10] = {0};
  UINT32 size = 0;
  char **strframe = NULL;
  UINT32 i = 0, j = 0;

  size = backtrace(array, 10);
  strframe = (char **)backtrace_symbols(array, size);

  printf("print call frame now:\n");
  for(i = 0; i < size; i++){
    printf("frame %d -- %s\n", i, strframe[i]);
  }

  if(strframe)
  {
    free(strframe);
    strframe = NULL;
  }
}

void fun2(void)
{
  fun3();
}

void fun1(void)
{
  fun2();
}

int main(void)
{
  fun1();
  return 0;
}
