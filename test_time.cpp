#include <stdio.h>
#include <time.h>
int main(int argc, const char* argv[])
{
        int i = 0;

      /*time_t rawtime;
      struct tm * timeinfo;

      time ( &rawtime );
      timeinfo = localtime( &rawtime );
      printf ( "Current local time and date: %s", asctime (timeinfo) );*/
        
        //sprintf();
        time_t tt = time(0);
        //产生“YYYY-MM-DD hh:mm:ss”格式的字符串。
        char s[32];
        i = strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", localtime(&tt));
        printf("\n i= %d\n",i);
        //s[i] = '\n';
        s[31] = '\n';
        printf("%s\n",s);
    return 0;
}