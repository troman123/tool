
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>


static void WidebrightSegvHandler(int signum)
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i, j;

	signal(signum, SIG_DFL); /* 还原默认的信号处理handler */

	size = backtrace (array, 10);
	strings = (char **)backtrace_symbols (array, size);

	fprintf(stderr, "widebright received SIGSEGV! Stack trace:\n");
	for (i = 0; i < size; i++) {
		fprintf(stderr, "%d %s \n", i, strings[i]);
	}
	
	free (strings);
	exit(1);
}

int invalide_pointer_error(char * p)
{
	*p = 'd'; //让这里出现一个访问非法指针的错误
	return 0;
}

void error_2(char * p)
{
	invalide_pointer_error(p);
}

void error_1(char * p)
{
	 error_2(p);
}

void error_0(char * p)
{
	 error_1(p);
}

int main() 
{
	//设置 信好的处理函数
	signal(SIGSEGV, WidebrightSegvHandler); // SIGSEGV	  11	   Core	Invalid memory reference
	signal(SIGABRT, WidebrightSegvHandler); // SIGABRT	   6	   Core	Abort signal from
	
	char *a = NULL;
	error_0(a);
	exit(0);
}