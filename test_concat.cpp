#include <stdio.h>
int main(int argc, char const *argv[])
{
    char fmt_buf[1024] = {0};
    #define __STR_CONCAT__ "COUNT=%d FIRST:%lld, LAST:%lld\n"
    sprintf(fmt_buf, "[%d:%d] AUDIO INPUT          :"__STR_CONCAT__\
                                "AUDIO INPUT          :"__STR_CONCAT__\
                             
                             , 1, 1, 1, 1, 1, 1);

    fprintf(stdout, "fmt_buf=%s", fmt_buf);
                                
    return 0;
}