#include <stdio.h>
#include <math.h>

void fun(int* a)
{
    delete a;
}

int main(int argc, char const *argv[])
{
    int a = 10;
    fun(&a);
    fprintf(stdout, "%f\n", ceil((float)3/2));
    return 0;
}