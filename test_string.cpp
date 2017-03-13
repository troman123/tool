#include <stdio.h>
#include <string>
using namespace std;

// string fun(char * str)
// {
//     return string(str);
// }

#define CLS_FUN_LINE(_cls_fun_line_) (sprintf(&_cls_fun_line_[0], "%s %s %d", "__CLASS__", __FUNCTION__, __LINE__), _cls_fun_line_.c_str())

void printf_log(const char *str)
{
    fprintf(stdout, "\nprintf_log %s\n", str);
}

int main(int argc, char const *argv[])
{
    //string a,;
    std::string str;
    char buf[20] = {0};
    sprintf(&str[0], "%dx%d", 400, 300);
    //str = buf;
    std::string log;
    fprintf(stdout, "%s str=%s\n", CLS_FUN_LINE(std::string()), str.c_str());
    printf_log(CLS_FUN_LINE(std::string()));
    return 0;
}