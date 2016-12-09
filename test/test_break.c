#include <stdio.h>

int main(int argc, char const *argv[])
{
    int count = 0;
    while (count++ < 10)
    {
        fprintf(stdout, "while\n");

        switch(1)
        {
            case 1:
                fprintf(stdout, "switch\n");
                break;
            default:
                break;
        }
    }
    return 0;
}