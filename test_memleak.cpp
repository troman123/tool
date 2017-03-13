#include <iostream> 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
using namespace std; 
#define _LINE_LENGTH 300 
 
 
bool GetCpuMem(float &cpu, size_t &mem, int pid = -1, int tid = -1) 
{ 
    if (pid == -1)
    {
        pid = getpid();
    }

    bool ret = false; 
    char cmdline[100]; 
    sprintf(cmdline, "ps -o %%cpu,rss,%%mem,pid,tid -mp %d", pid); 
    FILE *file; 
    file = popen(cmdline, "r"); 
    if (file == NULL)  
    { 
        printf("file == NULL\n"); 
        return false; 
    } 
 
    char line[4096] = {0}; 
    float l_cpuPrec = 0; 
    int l_mem = 0; 
    float l_memPrec = 0; 
    int l_pid=0; 
    int l_tid=0; 
    if (fgets(line, _LINE_LENGTH, file) != NULL)  
    { 
    //  printf("1st line:%s",line); 
        if (fgets(line, _LINE_LENGTH, file) != NULL)  
        { 
    //      printf("2nd line:%s",line); 
            sscanf( line, "%f %d %f %d -", &l_cpuPrec, &l_mem, &l_memPrec, &l_pid ); 
            cpu = l_cpuPrec; 
            mem = l_mem/1024; 
            if( tid == -1 )
            {
                ret = true; 
            }
            else 
            { 
                while( fgets(line, _LINE_LENGTH, file) != NULL ) 
                { 
                    sscanf( line, "%f - - - %d", &l_cpuPrec, &l_tid ); 
    //              printf("other line:%s",line); 
    //              cout<<l_cpuPrec<<'\t'<<l_tid<<endl; 
                    if( l_tid == tid ) 
                    { 
                        printf("cpuVal is tid:%d\n",tid); 
                        cpu = l_cpuPrec; 
                        ret = true; 
                        break; 
                    } 
                } 
                if( l_tid != tid )
                {
                    printf("TID not exist\n"); 
                }
            } 
        } 
        else
        {
            printf("PID not exist\n"); 
        }
    } 
    else 
    {
        printf("Command or Parameter wrong\n"); 
    }
    pclose(file); 
    return ret; 
} 
 
int main(int argc, char** argv) 
{ 
    float cpu=0; 
    size_t mem=0; 
    int pid=0; 
    int tid=-1; 
    if( argc > 1 ) 
        pid = atoi(argv[1]); 
    else 
        pid = getpid(); 
    if( argc > 2 ) 
        tid = atoi(argv[2]); 
    while(1) 
    { 
        if( GetCpuMem( cpu, mem, pid, tid ) ) 
        { 
            printf("%%CPU:%.1f\tMEM:%dMB\n", cpu, mem); 
        } 
        else 
            printf("return false\n"); 
        break; 
        sleep(5); 
    } 
 
    return 0; 
} 

// #include <stdio.h>
// #include <unistd.h>

// typedef struct PACKED1         //定义一个cpu occupy的结构体
// {
//     char name[20];      //定义一个char类型的数组名name有20个元素
//     unsigned int user; //定义一个无符号的int类型的user
//     unsigned int nice; //定义一个无符号的int类型的nice
//     unsigned int system;//定义一个无符号的int类型的system
//     unsigned int idle; //定义一个无符号的int类型的idle
// }CPU_OCCUPY;

// typedef struct PACKED         //定义一个mem occupy的结构体
// {
//     char name[20];      //定义一个char类型的数组名name有20个元素
//     unsigned long total; 
//     char name2[20];
//     unsigned long free;                       
// }MEM_OCCUPY;

// void get_memoccupy (MEM_OCCUPY *mem) //对无类型get函数含有一个形参结构体类弄的指针O
// {
//     FILE *fd;          
//     int n;             
//     char buff[256];   
//     MEM_OCCUPY *m;
//     m=mem;
                                                                                                              
//     fd = fopen ("/proc/meminfo", "r"); 
      
//     fgets (buff, sizeof(buff), fd); 
//     fgets (buff, sizeof(buff), fd); 
//     fgets (buff, sizeof(buff), fd); 
//     fgets (buff, sizeof(buff), fd); 
//     sscanf (buff, "%s %lu %s", m->name, &m->total, m->name2); 
    
//     fgets (buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里 
//     sscanf (buff, "%s %u %s", m->name2, &m->free, m->name2); 
    
//     fprintf(stdout, "get_cpuoccupy %s\n", buff);
//     fclose(fd);     //关闭文件fd
// }

// int cal_cpuoccupy (CPU_OCCUPY *o, CPU_OCCUPY *n) 
// {   
//     unsigned long od, nd;    
//     unsigned long id, sd;
//     int cpu_use = 0;   
    
//     od = (unsigned long) (o->user + o->nice + o->system +o->idle);//第一次(用户+优先级+系统+空闲)的时间再赋给od
//     nd = (unsigned long) (n->user + n->nice + n->system +n->idle);//第二次(用户+优先级+系统+空闲)的时间再赋给od
      
//     id = (unsigned long) (n->user - o->user);    //用户第一次和第二次的时间之差再赋给id
//     sd = (unsigned long) (n->system - o->system);//系统第一次和第二次的时间之差再赋给sd
//     if((nd-od) != 0)
//     cpu_use = (int)((sd+id)*10000)/(nd-od); //((用户+系统)乖100)除(第一次和第二次的时间差)再赋给g_cpu_used
//     else cpu_use = 0;
//     //printf("cpu: %u/n",cpu_use);
//     return cpu_use;
// }

// void get_cpuoccupy (CPU_OCCUPY *cpust) //对无类型get函数含有一个形参结构体类弄的指针O
// {   
//     FILE *fd;         
//     int n;            
//     char buff[256]; 
//     CPU_OCCUPY *cpu_occupy;
//     cpu_occupy=cpust;
                                                                                                               
//     fd = fopen ("/proc/stat", "r"); 
//     fgets (buff, sizeof(buff), fd);
    
//     sscanf (buff, "%s %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle);
    
//     fprintf(stdout, "get_cpuoccupy %s\n", buff);
//     fclose(fd);     
// }

// int main()
// {
//     CPU_OCCUPY cpu_stat1;
//     CPU_OCCUPY cpu_stat2;
//     MEM_OCCUPY mem_stat;
//     int cpu;
    
//     //获取内存
//     get_memoccupy ((MEM_OCCUPY *)&mem_stat);
    
//     //第一次获取cpu使用情况
//     get_cpuoccupy((CPU_OCCUPY *)&cpu_stat1);
//     usleep(10 * 1000);
    
//     //第二次获取cpu使用情况
//     get_cpuoccupy((CPU_OCCUPY *)&cpu_stat2);
    
//     //计算cpu使用率
//     cpu = cal_cpuoccupy ((CPU_OCCUPY *)&cpu_stat1, (CPU_OCCUPY *)&cpu_stat2);
    
//     return 0;
// } 