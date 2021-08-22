/* xsym.c -- SAD symbol extraction.

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
     opt_rewrite = FALSE,		/* Rewrite .symbols */
     opt_supersede = TRUE;		/* Merge (0)/Supersede (1) */

/* Local variables */
int
     added_symbols = 0;			/* How many added symbols */

struct symbol
     *symtail = NULL;			/* Symbol chain end */


/* Print usage */
void usage(argc, argv)
    int argc;
    char **argv;
{
     fprintf(stderr, "usage: xsym (%s) [-smrl]\n", *argv);
     exit(1);
}


/* Look up symbol by value.
 * Always returns the first applicable symbol by index, or -1 if none.
 */
static sym(val)
    int val;
{
     int bot, top, cent;
     
     /* Look up comment */
     top = nsymbols-1;
     bot = 0;
     cent = nsymbols >> 1;
     
     for(; top >= bot; cent = bot + ((top-bot) >> 1))
	  if(val > symref[cent]->val)
	       bot = cent+1;
	  else if(val < symref[cent]->val)
	       top = cent-1;
	  else
	   {
		for(; cent > 0 && symref[cent-1]->val == val; cent--);
		return(cent);
	   }
     
     return(-1);
}

static char *byname(str)
    register char *str;
{
     while(*str && *str > '\040')
	  str++;
     return(str);
}

/* Extract symbol from buffer, return TRUE if any. */
static extract_user_sym(buf, strp, typep)
    register char *buf, *strp;
    int *typep;
{
     char *cp;
     
     if( !*buf || ((*buf != '=') && (strncmp(buf, "LABEL", 5))))
	  return(FALSE);
     
     if(*buf == '=')
	  buf++;
     else
      {
	   buf += 5;
	   buf = byspace(buf);
	   if(*buf == '(' || *buf == ')' || (strncmp(buf, "EQU",3)==0))
		return(FALSE);
      }
     cp = byname(buf);

     strncpy(strp, buf, strlen(buf) - strlen(cp));
     strp[strlen(buf)-strlen(cp)] = '\0';

     if(strlen(strp)==0)
	  return(FALSE);
     
     /* Ignore local names */
     if(strncmp(strp, "L_", 2)==0)
	  return(FALSE);
    
     debug_msg("Found id: %s\n", strp);

     /* Decide type */
     /* Depends heavily on the options used for sad!! */
     /* Do not modify sad.el options for sad !!! */

     if(strstr(cp, STR_UNSUPPORTED) != NULL)
	  *typep = T_SYM_NOHP;
     else if(strstr(cp, STR_VERSDEPND) != NULL)
	  *typep = T_SYM_REVDEP;
     else
	  *typep = T_SYM_HP;

     return(TRUE);
}


/* Merge one line of input.
 * The patterns we look out for are:
 * 
 * [1] xxxxx =symbol    sssss
 * [2] xxxxx LABEL symbol sssss
 * Where sssss is "", "( Unsupported )" or "( Version dependant")
 *       xxxxx is optional address
 *
 * All other lines are ignored.
 * 
 */
static void merge_sym_line()
{
     char lbuf[132], org_lbuf[sizeof lbuf], *lp, symid[132];
     int new_pc = 0, tagaddr, stype, eraseflag, addflag, symix;

     struct symbol *newsym;

     lbuf[0] = '\0';
     fgets(lbuf, sizeof lbuf, stdin);
     lineno++;
     
     strcpy(org_lbuf, lbuf);
     
     if(feof(stdin))
	  return;

     lp = lbuf;
     if(!*lp)
	  return;

     debug_msg("%s", lbuf);
     
     /* Explicit PC? */
     if(IS_HEX(*lp) || *lp == '#')
	  if(hexstrtoi(&lp, &pc))
	   {
		lp = byspace(lp);
		new_pc = TRUE;
	   }
     
     if(!*lp)
	  return;
     
     if(new_pc)
	  debug_msg("--> pc=%x\n", pc);

     eraseflag = addflag = FALSE;
     
     if(!(addflag = extract_user_sym(lp, symid, &stype)))
	if(opt_supersede && new_pc)
	       eraseflag = TRUE;
	  else
	       return;

     tagaddr = (new_pc ? pc : determine_pc(stdin));     
     
     debug_msg("--> erase=%d, add=%d, new_pc=%d, tagaddr=%x, type=%02X\n",
	       eraseflag, addflag, new_pc, tagaddr, stype);
      
     if((eraseflag || opt_supersede) && (symix = sym(tagaddr)) >= 0)
	  for(; symref[symix] && symref[symix]->val == tagaddr; symix++)
	   {
		debug_msg("--> #%d ", symix);
		
		/* Erase if old symbol */
		if(addflag)
		 {
		      if(!(symref[symix]->type & (T_SYM_ERASE|T_SYM_ISDEF)))
		       {
			    symref[symix]->type |= T_SYM_ERASE;
			    debug_msg("erased old ");
		       }
		      else
			   debug_msg("didn't erase old ");
		 }
		else
		 {
		      if(!(symref[symix]->type & (T_SYM_ERASE|T_SYM_ISDEF)))
		       {
			    symref[symix]->type |= T_SYM_ERASE;
			    debug_msg("erased ");
		       }
		      else
			   debug_msg("didn't erase ");
		 }
		
		debug_msg("\"%s\"\n", symref[symix]->id);
		
		if((symref[symix]->type & T_SYM_ERASE) && symref[symix]->id)
		 {
		      free(symref[symix]->id);
		      symref[symix]->id = NULL;
		 }
	   }
		
     if(!addflag)
	  return;

    /* Finally, we stick in our defs last in the database */
    if(!(newsym = (struct symbol *) malloc(sizeof(struct symbol))))
      {
	   perror("symbol malloc");
	   return;
      }
		
     newsym->link = symroot;
     symroot = newsym;

     added_symbols++;

     newsym->id   = strdup(symid);
     newsym->type = stype | T_SYM_ISDEF;
     newsym->val  = tagaddr;
     newsym->seq  = nsymbols + added_symbols;
     newsym->ref = 0;
     newsym->xrefhead = newsym->xreftail = NULL;
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
		 case 'Q':		/* Debug info */
		      opt_debug = !opt_debug;
		      break;
		 case 's':		/* Supersede */
		      opt_supersede = TRUE;
		      break;
		 case 'm':		/* Merge */
		      opt_supersede = FALSE;
		      break;
		 case 'r':		/* Rewrite .symbols */
		      opt_rewrite = !opt_rewrite;
		      break;
		 case 'l':		/* Line numbers in errors and warnings */
		      opt_lines = !opt_lines;
		      break;
		 default:
		      usage(argc, argv);
		 }
      }
     
     if(argc != 1+flagsp)
	  usage(argc, argv);
     
     debug_msg("Debugging is enabled.\nThis is SAD%s %s\n\n", 
	       sad_version, *argv);
}  


main(argc, argv)
    int argc;
    char **argv;
{
     init();			/* Initialize tables */
     
     args(argc, argv);		/* Decode arguments */
     
     load_symbols(symcmp);

     pc = 0;
     while(!feof(stdin))
	  merge_sym_line();		/* Merge/supersede one line of input */
     
     /* Sort by address again to add new ones */
     nsymbols += added_symbols;
     create_symref(symcmp);

     /* Print new symbol file */
     print_symfile((opt_rewrite ? SAD_SYMBOLS : NULL));

}

