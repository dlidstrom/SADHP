/* Convert dump to core.
 *
 */
#include <stdio.h>

main()
{
  char data[132], *cp, *bp, wbuf[40];
  FILE *corefile;
  int nd;


  if(!(corefile = fopen(".core", "w")))
    {
      perror(".core");
      exit(1);
    }

  while(!feof(stdin))
    {
      /* Read */
      gets(data);
      gets(data);

      if(feof(stdin))
	{
	  fclose(corefile);
	  break;
	}

      nd = strlen(data);

      /* memset(wbuf, '\0', 16); */
      /* bclear(wbuf, 16); */

      for(bp = wbuf; bp < wbuf+16; *bp++ = '\0');

      for(bp = wbuf, cp = data+nd-1; cp > data+2;)
	{
	  cp--;
	  *bp++ = (*cp > '9' ? *cp - 'A' + 10 : *cp - '0');
	}

      fwrite(wbuf, 16, 1, corefile);
    }
}
