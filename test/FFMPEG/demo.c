#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include "demo.h"

int main(int argc, char *argv[])
{
    av_register_all();

    printf("main\n");
}