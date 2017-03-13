#include <stdio.h>
#include <pthread.h>
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <unistd.h>
#include <sys/syscall.h>

void* printid(void *ptr)
{
    printf("gettid A The id of %s is %u\n",(char*)ptr,syscall( __NR_gettid ));
    printf("pthread_self B The id of %s is %u\n",(char*)ptr,(unsigned int)pthread_self());
}

int main()
{
    pthread_t thread[5];
    char *msg[]={"thread1","thread2","thread3","thread4","thread5"};
    int i;
    for(i=0;i<5;i++)
    {
        pthread_create(&thread[i],NULL,printid,(void*)msg[i]);
        pthread_join(thread[i],NULL);
    }
    return 0;
}