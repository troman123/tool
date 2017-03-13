#include <stdio.h>


#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define FFERRTAG(a, b, c, d) (-(int)MKTAG(a, b, c, d))
#define AVERROR_EOF                FFERRTAG( 'E','O','F',' ') ///< End of file


int main(int argc, char const *argv[])
{
    fprintf(stdout, "%d\n", AVERROR_EOF);
    return 0;
}