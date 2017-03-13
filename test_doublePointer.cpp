#include <stdio.h>
void fun(int **a)
{
    if (a == NULL)
    {
        fprintf(stdout, "a is NULL \n");
    }
    else if (*a == NULL)
    {
        fprintf(stdout, "*a is NULL \n");
    }
    else
    {
        fprintf(stdout, "**a=%d\n", **a);
    }
}

int main(int argc, char const *argv[])
{
    int *a = NULL;
    fun(&a);

    fprintf(stdout, "******************** \n");

    fun(NULL);

    fprintf(stdout, "******************** \n");

    delete a;
    return 0;
}