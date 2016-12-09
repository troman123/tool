#include <stdio.h>

int main(int argc, char const *argv[])
{
    void *ptr = NULL;
    int ret = init(&ptr);
    if (ret == 0)
    {
        readPkt(ptr);
    }
    else
    {
        fprintf(stdout, "init ret failed. ret=%d\n", ret);
    }

    return 0;
}