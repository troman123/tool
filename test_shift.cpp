#include <stdio.h>
#include <stdint.h>

int main(int argc, char const *argv[])
{
    int64_t sf = 0;
    sf -= 1ULL << 32;
    fprintf(stderr, "%lld\n", sf);
    return 0;
}