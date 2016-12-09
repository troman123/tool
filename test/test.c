#include <stdio.h>
#include <string.h>
int main(int argc, char const *argv[])
{
	char *line = "#EXTINF:6.480000,";

	const char *str = "1,2,3,4";
	char line1[10] = {0};
	memcpy(line1, str, strlen(str));
	char *buf = line1;
	//fprintf(stdout, "%s\n", buf);


	char *tmp = strtok(buf, ",");
	while( tmp != NULL )
	{
		fprintf(stdout, "%s\n", tmp);
		tmp = strtok(NULL, ",");
	}

	return 0;
}


// int main ()
// {
//   char str[] ="- This, a sample string.";
//   char * pch;
//   printf ("Splitting string \"%s\" into tokens:\n",str);
//   pch = strtok (str," ,.-");
//   while (pch != NULL)
//   {
//     printf ("%s\n",pch);
//     pch = strtok (NULL, " ,.-");
//   }
//   return 0;
// }


void Test_strtok_r()
{
    char buffer[INFO_MAX_SZ]="Fred male 25,John male 62,Anna female 16";
    char *p[20];
    char *buf=buffer;
    char *outer_ptr=NULL;
    char *inner_ptr=NULL;

    while((p[in] = strtok_r(buf, ",", &outer_ptr))!=NULL)
    {
        buf=p[in];
        while((p[in]=strtok_r(buf, " ", &inner_ptr))!=NULL)
        {
            in++;
            buf=NULL;
        }
        buf=NULL;
    }

    printf("Here we have %d strings/n",in);
    for (int j=0; j<in; j++)
    {
        printf(">%s</n",p[j]);
    }
}