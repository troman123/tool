#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <execinfo.h>
#include <signal.h>


//signal 函数用法参考http://www.kernel.org/doc/man-pages/online/pages/man2/signal.2.html
//backtrace ，backtrace_symbols函数用法参考 http://www.kernel.org/doc/man-pages/online/pages/man3/backtrace.3.html

static void widebrightSegvHandler(int signum)
{
    void *array[10];
    size_t size;
    char **strings;
    size_t i, j;

    signal(signum, SIG_DFL); /* 还原默认的信号处理handler */

    size = backtrace(array, 10);
    strings = (char **)backtrace_symbols(array, size);

    fprintf(stderr, "widebright received SIGSEGV! Stack trace:\n");

    for (i = 0; i < size; i++)
    {
        fprintf(stderr, "%d %s \n",i,strings[i]);
    }

    free (strings);

    exit(1);
}

int invalide_pointer_error(char * p)
{
    *p = 'd'; //让这里出现一个访问非法指针的错误
    return 0;
}


void error_2(char * p)
{
    invalide_pointer_error(p);
}

void error_1(char * p)
{
     error_2(p);
}

void error_0(char * p)
{
     error_1(p);
}

//friend class ::FriendTest;
namespace test {
    class Test;
};

namespace test {
class Test
{
public:
    Test(){
        tag = 1;
    }
    ~Test(){}

private:
    friend class FriendTest;

    int tag;
    /* data */
};

}

//namespace test {
//using namespace test;
class FriendTest
{
public:
    FriendTest()
    {
        using test::Test;
        //using namespace test;
 
        // Test tt;
        // int tag = tt.tag;
        // printf("tag=%d\n", tag);
    }
    ~FriendTest()
    {

    }

    /* data */
};

//}

#include <iostream>
namespace my
{
    int a=3;
}

class test2
{
public:
    int a;
    
    test2(){a=1;}

    void fb()
    {
        int a=0;
        std::cout<<(my::a)<<(test2::a)<<a<<std::endl;
    }
};

// int main()
// {
// test t;
// t.fb();
// return 0;
// }



//forward declaration
namespace nsb
{
    class B;
}

namespace nsa {

class A
{
public:
    A(): num_(0) {}
    int get_num() const { return num_; }

    friend class nsb::B;
    //friend class ::B; //if B is in global namespace, need to use "::B"

private:
    int num_;
};

} // namespace


namespace nsa {
    class A;
}

namespace nsb {

class B
{
public:
    B(nsa::A *a);
    void set_num_of_A(int n);
    int get_num_of_A() const;

private:
    nsa::A *pa_;
};

} // namespace



namespace nsb {

    B::B(nsa::A *pa): pa_(pa)
    {
    }

    void B::set_num_of_A(int n)
    {
        pa_->num_ = n;
    }

    int B::get_num_of_A() const 
    {
        return pa_->num_;
    }

}

int main() 
{

    // //设置 信好的处理函数,各种 信号的定义见http://www.kernel.org/doc/man-pages/online/pages/man7/signal.7.html
    // signal(SIGSEGV, widebrightSegvHandler);
    // // SIGSEGV      11       Core    Invalid memory reference
    // signal(SIGABRT, widebrightSegvHandler);
    // // SIGABRT       6       Core    Abort signal from

    // unsigned d=32768;
    // printf("a=%d\n", d);

    // char *a = NULL;
    // error_0(a);
    // exit(0);
    //using test::FriendTest;

    FriendTest ft;


    test2 t;
    t.fb();
}
