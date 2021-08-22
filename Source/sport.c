#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>

int main(argc, argv)
    int argc;
    char **argv;
{ 
     int value, counter, chr;
     FILE *in, *out;

     value = counter = 0;
     
     if(!(in=fopen("port.bin","rb")))
      {
	   perror("port.bin");
	   exit(1);
      }
     if(!(out=fopen(".port1","wb")))
      {
	   perror(".port1");
	   exit(1);
      }
     while( !feof( in ) ) {
	chr = getc(in);
	if((counter++)>=8)
	    {
		 value= chr & 0xF;
		 putc(value,out);
		 value=(chr/16) & 0xF ;
		 putc(value,out);
	    }
      }
     return 0 ; 
}
