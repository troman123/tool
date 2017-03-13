extern "C" {
    #include <stdio.h>
    #include <sys/types.h>
 
    #include <unistd.h>
    #include <sys/syscall.h>   /* For SYS_xxx definitions */
    #include <sys/types.h>
}

int main(int argc, char const *argv[])
{
    fprintf(stdout, "%ld double size=%d\n", syscall( __NR_gettid ), sizeof(double));
    return 0;
}