/* xcom.c -- SAD commment extraction

This file is part of SAD, the Saturn Disassembler package.

SAD is not distributed by Free Software Foundation. Do not ask them
for a copy or how to obtain new releases. Instead, send e-mail to the
address below. SAD is merely covered by the GNU General Public
License.

Please send your comments, ideas, and bug reports to
Jan Brittenson <bson@ai.mit.edu>

*/


/* Copyright (C) 1990 Jan Brittenson.

SAD is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 1, or (at your option) any later
version.

SAD is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with SAD; see the file COPYING.  If not, write to the Free
Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */


#include <stdio.h>
#include "globals.h"

/* Options */
int
     opt_rewrite = FALSE,	/* Rewrite .comments */
     opt_supersede = TRUE;		/* Merge (0)/Supersede (1) */

int
     added_coms = 0;


struct symbol
     *comtail = NULL;		/* Comment chain end */


/* Create comment reference array, end with NULL guard */
void create_comref(compfunc)
    int (*compfunc)();
{
     struct symbol **comrefptr, *comptr;
     
     if(comref)
	  free(comref);
     
     comref = (struct symbol **) malloc(sizeof(struct symbol *) * (ncomments+1));
     
     for(comrefptr = comref, comptr = comroot; comptr; comptr = comptr->link)
	  *comrefptr++ = comptr;
     
     *comrefptr = NULL;
     
     /* Sort the reference array in ascending value order */
     qsort((char *) comref, ncomments, sizeof(struct symbol *), compfunc);
}


/* Load comments. */
static void load_comments(comfname)
    char *comfname;
{
     FILE *comfile;
     char lbuf[132], *cp;
     struct symbol *comtemp;
     int seq;
     
     if(!(comfile = fopen(comfname, "r")))
	  return;
     
     comroot = comtail = NULL;
     for(ncomments = seq = 0; !feof(comfile); )
      {
	   fgets(lbuf, sizeof lbuf, comfile);
	   if(feof(comfile))
		break;
	   
	   lbuf[strlen(lbuf)-1] = '\0';
	   
	   if(!(comtemp = (struct symbol *) malloc(sizeof(struct symbol))))
	    {
		 perror("comment alloc");
		 exit(1);
	    }
	   
	   comtemp->link = comroot;
	   comroot = comtemp;
	   
	   cp = lbuf;
	   hexstrtoi(&cp, &comtemp->val);
	   
	   if(!*cp)
	    {
		 comroot = comtemp->link;
		 free(comtemp);
		 continue;
	    }
	   
	   comtemp->seq = ++seq;
	   comtemp->type = (*cp++ == '=' ? T_COM_MAJOR : T_COM_MINOR);
	   comtemp->id = strdup(cp);
	   ncomments++;
	   
	   if(!comtail)
		comtail = comtemp;
      }
     
     fclose(comfile);
     
     create_comref(comcmp);
}


/* Look up comment by value.
 * Always returns the first applicable comment by index, or -1 if none.
 */
static com(val)
    int val;
{
     int bot, top, cent;

     /* Look up comment */
     top = ncomments-1;
     bot = 0;
     cent = ncomments >> 1;
     
     for(; top >= bot; cent = bot + ((top-bot) >> 1))
	  if(val > comref[cent]->val)
	       bot = cent+1;
	  else if(val < comref[cent]->val)
	       top = cent-1;
	  else
	   {
		for(; cent > 0 && comref[cent-1]->val == val; cent--);
		return(cent);
	   }
     
     return(-1);
}


/* Print usage */
void usage(argc, argv)
    int argc;
    char **argv;
{
     fprintf(stderr, "usage: xcom (%s) [-smrl]\n", *argv);
     exit(1);
}


/* Extract comment from buffer, return TRUE if any. */
static extract_user_com(buf, strp, typep)
    register char *buf, **strp;
    int *typep;
{
     int cpos;
     char *cp;
     
     if(*buf && *buf == '*')
      {
	   buf++;
	   /* Ignore "*****.." */
	   if((*buf) && (*buf == '*'))
		return(FALSE);

	   /* We have a possible major comment */
	   *typep = T_COM_MAJOR;

	   if(!(*buf) || (*buf != ' '))
		return(FALSE);

	   buf++;
	   if(!(*buf))
		return(FALSE);

	   /* All after "* " is major comment */
	   *strp = buf;
	   return(TRUE);
      }

     /* Minor comment or nothing */

     *typep = T_COM_MINOR;

     /* Skip until " * " or "\t* " */
     for(cpos = 0;
	 *buf && strncmp(buf," * ",3) && strncmp(buf,"\t* ",3);
	 buf++)
	  if(*buf == '\t')
	       cpos += 8;
	  else
	       cpos++;

     if(!*buf || !*++buf || !*++buf || !*++buf)
	  return(FALSE);

     /* Scan until end of line or comment made by sad */
     for(cp = buf; *cp && strncmp(cp, "*_",2); cp++);
     
     if(*cp == ' ') cp--;
     
     while(cp > buf && *cp <= '\040') cp--;
     
     *++cp = '\0';
     
     if(cp == buf+1)
	  return(FALSE);
     
     *strp = buf;
     
     return(TRUE);
}


/* Merge one line of input. */
static void merge_com_line()
{
     char lbuf[132], *lp, *comstr;
     int new_pc = 0, ctype, tagaddr, eraseflag, addflag, comix;
     struct symbol *newcom;
     
     
     lbuf[0] = '\0';
     fgets(lbuf, sizeof lbuf, stdin);
     lineno++;
     
     lbuf[strlen(lbuf)-1] = '\0';
     
     if(feof(stdin))
	  return;
     
     lp = lbuf;
     if(!*lp)
	  return;
     
     if(opt_debug)
	  fprintf(stderr, "%s\n", lbuf);
     
     /* Explicit PC? */
     if(IS_HEX(*lp) || *lp == '#')
	  if(hexstrtoi(&lp, &pc))
	       new_pc = TRUE;
     
     if(!*lp)
	  return;
     
     eraseflag = addflag = FALSE;
     
     /* Extract comment, if none check if need to erase */
     if(!(addflag = extract_user_com(lp, &comstr, &ctype)))
	  if((opt_supersede && new_pc) || ((ctype & T_COM_TYPE)==T_COM_MAJOR))
	       eraseflag = TRUE;
	  else
	       return;

     tagaddr = (new_pc || ctype == T_COM_MINOR ? pc : determine_pc(stdin));
     
     if(opt_debug)
	  fprintf(stderr, "--> eraseflag=%d, addflag=%d, new_pc=%d, tagaddr=%x, type=%02X\n",
		  eraseflag, addflag, new_pc, tagaddr, ctype);
     
     /* Loop all comments with suitable type, if any */
     if((eraseflag || opt_supersede) && (comix = com(tagaddr)) >= 0)
	  for(; comref[comix] && comref[comix]->val == tagaddr; comix++)
	   {
		if(opt_debug)
		     fprintf(stderr, "--> #%d ", comix);
		
		/* Erase if old comment */
		if(addflag)
		 {
		      if((comref[comix]->type & T_COM_TYPE) == ctype &&
			 !(comref[comix]->type & (T_COM_ERASE|T_COM_ISDEF)))
		       {
			    comref[comix]->type |= T_COM_ERASE;
			    if(opt_debug)
				 fputs("erased old ", stderr);
		       }
		      else if(opt_debug)
			   fputs("didn't erase old ", stderr);
		 }
		else
		 {
		      if(!(comref[comix]->type & (T_COM_ERASE|T_COM_ISDEF)))
		       {
			    comref[comix]->type |= T_COM_ERASE;
			    if(opt_debug)
				 fputs("erased ", stderr);
		       }
		      else if(opt_debug)
			   fputs("didn't erase ", stderr);
		 }
		
		if(opt_debug)
		     fprintf(stderr, "\"%s\"\n", comref[comix]->id);
		
		if((comref[comix]->type & T_COM_ERASE) && comref[comix]->id)
		 {
		      free(comref[comix]->id);
		      comref[comix]->id = NULL;
		 }
	   }
     
     if(!addflag)
	  return;
     
     /* Finally, we stick in our defs last in the database */
     if(!(newcom = (struct symbol *) malloc(sizeof(struct symbol))))
      {
	   perror("comment malloc");
	   return;
      }

     newcom->link = NULL;
     if(comtail)
	  comtail->link = newcom;
     
     comtail = newcom;
     
     added_coms++;

     newcom->id   = strdup(comstr);
     newcom->type = ctype | T_COM_ISDEF;
     newcom->val  = tagaddr;
     newcom->seq  = ncomments + added_coms;
}


/* Parse arguments */
static void args(argc, argv)
    int argc;
    char **argv;
{
     char *cp1;
     int flagsp = 0;
     
     if(argc > 1 && argv[1][0] == '-')
      {
	   flagsp = 1;
	   
	   for(cp1 = argv[1]+1; *cp1; cp1++)
		switch(*cp1)
		 {
		 case 'Q':		/* Debug */
		      opt_debug = !opt_debug;
		      break;
		 case 's':		/* Supersede */
		      opt_supersede = 1;
		      break;
		 case 'm':		/* Merge */
		      opt_supersede = 0;
		      break;
		 case 'r':		/* Rewrite .comments */
		      opt_rewrite = !opt_rewrite;
		      break;
		 case 'l':		/* Include line info */
		      opt_lines = !opt_lines;
		      break;
		 default:
		      usage(argc, argv);
		 }
      }
     
     
     if(argc != 1+flagsp)
	  usage(argc, argv);
     
     if(opt_debug)
	  fprintf(stderr, "Debug mode enabled.\nThis is SAD %s %s\n",
		  sad_version, *argv);
}  


main(argc, argv)
    int argc;
    char **argv;
{
     init();			/* Initialize tables */
     
     args(argc, argv);		/* Decode arguments */
     
     load_comments(SAD_COMMENTS);	/* Load symbol table */

     pc = 0;
     while(!feof(stdin))
	  merge_com_line();		/* Merge/supersede one line of input */

     ncomments += added_coms;
     /* Gotta create comref again in order to print in addr order */
     create_comref(comcmp);

     /* Print new symbol file */
     print_comfile((opt_rewrite ? SAD_COMMENTS : NULL)); 
}
