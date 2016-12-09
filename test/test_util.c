#include <stdio.h>
#include <stddef.h>
#include <ctype.h>

typedef struct _test_offsetof_
{
    int a;
    char b;
    int c;
} _test_offsetof_;

void Test_offsetof()
{
    _test_offsetof_ t1;
    fprintf(stdout, "%lu\n", offsetof(__typeof__(t1), a));
    fprintf(stdout, "%lu\n", offsetof(__typeof__(t1), b));
    fprintf(stdout, "%lu\n", offsetof(__typeof__(t1), c));
    fprintf(stdout, "%lu\n", sizeof(_test_offsetof_));
}

void Test_atoi()
{
    const char *s = "123456";
    int i, n, sign;
    for(i = 0; isspace(s[i]); i++)//跳过空白符;
    {
        sign = (s[i] == '-') ? -1 : 1;
    }
    if(s[i] == '+' || s[i] == '-')//跳过符号
    {
        i++;
    }
    for(n = 0; isdigit(s[i]); i++)
    {
        n = 10 * n + (s[i] - '0');//将数字字符转换成整形数字
    }

    //return sign * n;
    fprintf(stdout, "test atoi. ret=%d\n", sign * n);
}


void Test_itoa()//
{
}

void Test_const()
{
    int i;
    int * const p = &i;
    int *q;
    q = p;
}

void Test_string()
{

}

int main(int argc, char const *argv[])
{
    Test_offsetof();
    return 0;
}