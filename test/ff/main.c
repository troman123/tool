#include <stdio.h>

static int case_type = 1;
int main(int argc, char const *argv[])
{
    fprintf(stdout, "main run case_type=%d\n", case_type);

    switch(case_type)
    {
        case 0:
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
            break;
        }
        case 1:
        {
            outoutmedia(1, NULL);
            break;
        }
        case 2:
        {
            audiodecodedemo(1, NULL);
            break;
        }
    }

    return 0;
}