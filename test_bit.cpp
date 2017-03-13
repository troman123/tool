#include <stdio.h>
#include <stdint.h>

int main(int argc, char const *argv[])
{
    uint32_t i1 = 3550152699;
    uint32_t i2 = 4294922145;

    uint32_t s1 = i1 + i2;
    int32_t s2 = i1 + i2;

    fprintf(stdout, "%lu %d\n", s1, s2);//3550107548
    return 0;
}