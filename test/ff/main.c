#include <stdio.h>

int main(int argc, char const *argv[])
{
    if (0)
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
    }
    else
    {
        outoutmedia(1, NULL);
    }
    

    return 0;
}