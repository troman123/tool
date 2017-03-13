#include <stdio.h>

class TestChild
{
public:
    TestChild()
    {
        x=0;
        y=0;
        printf("TestChild: Constructor be called!\n");
    }
    ~TestChild(){}
    TestChild(const TestChild& tc)
    {
        x=tc.x;
        y=tc.y;
        printf("TestChild: Copy Constructor called!//因为写在了Test(拷贝)构造函数的初始化列表里\n");
    }
    
    const TestChild& operator=(const TestChild& right)
    {
        x=right.x;
        y=right.y;
        printf("TestChild: Operator = be called! //因为写在了Test(拷贝)构造函数的函数体里\n");
        return *this;
    }

    int x,y;
};

class Test
{
public:

    Test(){printf("Test:      Constructor be called!\n");}
    explicit Test(const TestChild& tcc)
    {
        tc=tcc;
    }
    ~Test(){}
    Test(const Test& test):tc(test.tc)
    {
        tc=test.tc;
        printf("Test:      Copy Constructor be called!\n");
    }

    const Test & operator=(const Test& right)
    {
        tc=right.tc;
        printf("Test:      Operator= be called!\n");
        return *this;
    }

    TestChild tc;
};

int main()
{
    printf("1、Test中包含一个TestChild，这两个类分别具有构造函数、\n   拷贝构造函数、重载operator=。\n\n");
    printf("2、在调用Test的构造函数和拷贝构造函数之前，会根据跟在\n   这些函数后的初始化列表去初始化其\n   TestChild变量（调用TestChild的拷贝构造函数去初始化）\n\n");
    printf("3、一旦进入Test的构造函数体或拷贝构造函数体，则说明其成员变量TestChild已\n   经通过TestChild的构造函数或TestChild的拷贝构造函数构造出了对象\n");
    printf("   所以，在Test的构造函数体或拷贝构造函数体中，再去使用=号\n   给TestChild的时候，调用的就是TestChild的operator=，\n   而不是TestChild的拷贝构造函数了\n");
    printf("   这就是Test构造函数后面 “:” 初始化列表的存在意义！（\n   为了调用成员变量的构造函数或者拷贝构造函数）\n\n");
    printf("4、最后！揪出让人困惑的终极原因！！！！！\n   Test test2=test1和Test test2(test1)这两种是TM一模一样的\n   （都调用拷贝构造函数）！！！！除了这点儿之外，其他地方都是该是什么是什么（\"()\"调用构造函数，\"=\"调用赋值操作符）！！！\n\n");
    printf("5、一个对象初始化完毕后，所有对这个对象的赋值都调用operator=\n\n输出如下：");

    printf("Test test1; DO:\n");
    Test test1;
    printf("\n");
    printf("Test test2=test1; DO:\n");
    Test test2=test1;
    printf("\n");
    printf("Test test3(test2); DO:\n");
    Test test3(test2);
    printf("\n");
    printf("test3=test1; DO:\n");
    test3=test1;

     return 0;
}