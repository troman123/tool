#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base64.h"

int
main(void)
{
	unsigned char input[] = "pleasure.";
	unsigned char decode[1000];
	char output[13];
	bzero(output, sizeof(output));
	base64_encode(input, sizeof(input) - 1, output);

    fprintf(stdout, "%s\n", &output);
    const char *url2 = "http%3A%2F%2Fgodmusic.bs2dl.yy.com%2FdmE5ZDEyMDBiMzg2ZTM4MDE5YjY0ODBlY2MwMmJlOTU2ODE3MTQzN21j";
	base64_decode(url2, strlen(url2) - 1, decode);
    fprintf(stdout, "%s\n", &decode);

	return 0;
}

