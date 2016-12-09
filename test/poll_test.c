#include <sys/poll.h> 
#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>

static int count = 0;

void *start_routine (void *param)
{
    int wfd = *((int*)param);

    if (wfd > 0)
    {
        write (wfd, &(uint64_t){ 1 }, sizeof (uint64_t));
    }

    // while(count == 0)
    // {
    //     fprintf(stdout, "usleep\n");
    //     usleep(1000000);
    //     continue;
    // }

    // if (wfd > 0)
    // {
    //     write (wfd, &(uint64_t){ 1 }, sizeof (uint64_t));
    // }

    fprintf(stdout, "start_routine out\n");
}

void test_pipe()
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        fprintf(stderr, "pipe fail.");
    }

    pthread_t pid;
    int pret = pthread_create(&pid, NULL,
                          start_routine, &pipefd[1]);
    if (pret != 0)
    {
        fprintf(stderr, "pthread_create fail.");
        return;
    }

    struct pollfd ufd[1] = {0};

reconnect:
    
    fprintf(stdout, "reconnect count=%d\n", count);
    ufd[0].fd = pipefd[0];
    ufd[0].events = POLLIN;
    // struct pollfd ufd[1] = {
    //             { .fd = pipefd[0],   .events = POLLIN },
    // };

    int ret = poll(ufd, sizeof(ufd) / sizeof(ufd[0]), -1);
    
    fprintf(stdout, "poll count=%d ret=%d\n", count, ret);
    if (ret > 0 && count++ < 10)
    {
        int a;
        read (pipefd[0], &a, sizeof(int));
        goto reconnect;
    }


    close(pipefd[0]);
    close(pipefd[1]);
}

int main(int argc, char const *argv[])
{
    test_pipe();
    return 0;
}