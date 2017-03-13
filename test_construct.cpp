#include <stdio.h>

struct A
{
    A(){};
private:
    A(const A& a);
    A& operator=(const A& a);
};

int main(int argc, char const *argv[])
{
    A a;
    A *a1 = NULL;
    *a1 = a;
    A *a2 = NULL;
    //*a2 = a;

    return 0;
}