/* formats.c -- SAD formats related code.

This file is part of SAD, the Saturn Disassembler package.

SAD is not distributed by the Free Software Foundation. Do not ask
them for a copy or how to obtain new releases. Instead, send e-mail to
the address below. SAD is merely covered by the GNU General Public
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
#include <sys/types.h>
#include <setjmp.h>

#include "formats.h"
#include "globals.h"

/* Convert any (< 10) base string to integer,
 * leave pointer at first nonhex char.
 * Adapted from misc.
 */
static fmt_anystrtoi(cpp, ip, base)
  register char **cpp;
  int *ip, base;
{
  register data;
  int syntax_ok = FALSE;
  char *save, basemax;

  
  if(!cpp || !*cpp)
    return(FALSE);

  save = *cpp;

  basemax = base + '0' - 1;
  for(data = 0; (**cpp >= '0' && **cpp <= basemax); (*cpp)++)
    {
      data *= base;
      data += **cpp-'0';
      syntax_ok = TRUE;
    }
  
  if(!syntax_ok)
    return(FALSE);

  *ip = data;
  return(TRUE);
}


/* Count number of partial descriptions.
 * This is assumed to be the number of commans plus 1.
 */
static count_descr_aux(strp, termc)
  register char **strp, termc;
{
  register ndescr = 1;


  for(; *strp && **strp && **strp != termc; (*strp)++)
    switch(**strp)
      {
      case '(':

	(*strp)++;
	count_descr_aux(strp, ')');
	break;

      case ',':

	ndescr++;
	break;
      }

  return(ndescr);
}

static count_descr(str, termc)
  char *str, termc;
{
  return(count_descr_aux(&str, termc));
}


/* Parse format specification. The following are all valid patterns:
 *
 * data =
 * [r](data)[,data]	- Subdescription
 * [r]x[w][,data]	- Hex
 * [r]d[w][,data]	- Decimal
 * [r]o[w][,data]	- Octal
 * [r]b[w][,data]	- Binary
 * [r]a[w][,data]	- Ascii
 *
 * c			- Code
 * r			- RPL
 *
 * Default values for r and w is 1.
 */

struct format *formatp(strp, termc)
  char **strp, termc;
{
  struct format *fmttemp;
  struct format_ent *curent;
  int rept, width, nfmt;

  if(!strp || !*strp || !**strp)
    return(NULL);
  
  if(**strp == 'c' ||
     **strp == 'r' ||
     **strp == 'h' ||
     **strp == 'l' ||
     **strp == 'L' )
    {
	 if(!(fmttemp = (struct format *) malloc(sizeof(struct format))))
	  {
	       perror("format alloc");
	       return(NULL);
	  }
	 
	 fmttemp->nformats = 1;
	 fmttemp->fmtspec[0].width = fmttemp->fmtspec[0].repeat = 1;
	 fmttemp->fmtspec[0].fmtdescr = NULL;
	 
	 switch(**strp)
	  {
	  case 'c':  fmttemp->fmtspec[0].ftype = FT_CODE; break;
	  case 'r':  fmttemp->fmtspec[0].ftype = FT_RPL; break;
	  case 'h':  fmttemp->fmtspec[0].ftype = FT_HASH; break;
	  case 'l':  fmttemp->fmtspec[0].ftype = FT_LINK; break;
	  case 'L':  fmttemp->fmtspec[0].ftype = FT_LIB; break;
	  }
	 return(fmttemp);
    }
  
  /* Calculate number of elements to allocate */
  nfmt = count_descr(*strp, termc);
  
  if(!nfmt)
       return(NULL);
  
  fmttemp = (struct format *) malloc(sizeof(struct format) +
				     sizeof(struct format_ent) *
				     (nfmt - 1));
  fmttemp->nformats = nfmt;
  
  /* Loop and pick formats one by one */
  for(curent = fmttemp->fmtspec; *strp && **strp && **strp != termc; curent++)
   {
	/* Possible repeat */
	if(!fmt_anystrtoi(strp, &curent->repeat, 10))
	     curent->repeat = 1;
	
	/* Determine type */
	curent->width = 1;
	curent->fmtdescr = NULL;
	
	*strp = byspace(*strp);
	switch(**strp)
	 {
	 case 'x': curent->ftype = FT_HEX; break;
	 case 'y': curent->ftype = FT_REL; break;
	 case 'z': curent->ftype = FT_SPCHAR; break;
	 case 'd': curent->ftype = FT_DEC; break;
	 case 'o': curent->ftype = FT_OCT; break;
	 case 'b': curent->ftype = FT_BIN; break;
	 case 'a': curent->ftype = FT_STR; break;
	 case 'A': curent->ftype = FT_STRLINE; break;
	 case 's': curent->ftype = FT_VSTR; break;
	 case 'S': curent->ftype = FT_VSTR2;break;
	 case 'v': curent->ftype = FT_VHEX; break;
	 case 'V': curent->ftype = FT_VHEX2; break;
	 case 'f': curent->ftype = FT_FLOAT; break;
	 case 'i': curent->ftype = FT_INSTR1; break;
	 case 'p': curent->ftype = FT_RPL1; break;
	 case 'g': curent->ftype = FT_GRBROW; break;
	 case '(':
	      /* Parse subdescription and set type */
	      (*strp)++;
	      if(curent->fmtdescr = formatp(strp, ')'))
	       {
		    curent->ftype = FT_SUBD;
		    break;
	       }
	 case '*':
	      /* This one allows comments in the formats file */
	      fmt_free(fmttemp);
	      return(NULL);
	 default:
	      
	      error("bad format description `%s' -- ignored.", *strp);
	      fmt_free(fmttemp);
	      return(NULL);
	 }
	
	(*strp)++;
	
	if(!(curent->ftype & FT_SUBD))
	 {
	      /* Parse any width info */
	      *strp = byspace(*strp);
	      
	      if(!fmt_anystrtoi(strp, &curent->width, 10))
		   if (curent->ftype != FT_STRLINE)
			curent->width = 1;
		   else
			curent->width = STRLINEMAX;
	 }
	if(--nfmt)
	     if(**strp == ',')
		  (*strp)++;
	     else
	      {
		   error("bad format syntax `%s' -- ignored.", *strp);
		   return(NULL);
	      }
   }
  return(fmttemp);
}


/* Deallocate format description */
void fmt_free(fmt)
  struct format *fmt;
{
  register struct format_ent *ent;

  if(!fmt || !fmt->nformats)
    return;

  /* First deallocate any subdescriptions */
  for(ent = fmt->fmtspec; ent < fmt->fmtspec + fmt->nformats; ent++)
    if(ent->ftype & FT_SUBD)
      fmt_free(ent->fmtdescr);

  /* Then deallocate node */
  free(fmt);
}


/* Compute size of format description */
static jmp_buf unknown_catch;

static fmt_nibbles_aux(fmt)
  struct format *fmt;
{
  register struct format_ent *ent;
  int size;

  if(!fmt || !fmt->nformats)
    longjmp(unknown_catch, TRUE);

  /* Loop and add */
  for(size = 0, ent = fmt->fmtspec;
      ent < fmt->fmtspec + fmt->nformats;
      ent++)

    if(ent->ftype & FT_SUBD)
      size += ent->repeat * fmt_nibbles_aux(ent->fmtdescr);
    else
	switch(ent->ftype & FT_DATA_MASK)
	  {
	  case FT_HEX:
	  case FT_DEC:
	  case FT_OCT:
	  case FT_BIN:

	    size += ent->repeat * ent->width;
	    break;

	  case FT_STR:

	    size += (ent->repeat * ent->width) << 1;
	    break;

	  default:

	    longjmp(unknown_catch, TRUE);
	  }

  return(size);
}


fmt_nibbles(fmt)
  struct format *fmt;
{
  if(!setjmp(unknown_catch))
    return(fmt_nibbles_aux(fmt));

  return(0);
}

  
/* Print format description to stdout.
 */
void fmt_print(fmt, termc, fp)
  struct format *fmt;
  char termc;
  FILE *fp;
{
  register struct format_ent *ent;

  if(!fmt)
    {
      fprintf(stderr, "[No format description]\n");
      return;
    }

  for(ent = fmt->fmtspec; ent < fmt->fmtspec + fmt->nformats; ent++)
    {
      if(ent->repeat != 1)
	fprintf(fp, "%d", ent->repeat);

      if(ent->ftype & FT_SUBD)
	{
	  fputc('(', fp);
	  fmt_print(ent->fmtdescr, ')', fp);
	}
      else
	{
	  if(ent->ftype & FT_CODE)
	    fputc('c', fp);
	  else
	    if(ent->ftype & FT_RPL)
	      fputc('r', fp);
	    else
	      switch(ent->ftype & FT_DATA_MASK)
		{
		case FT_HEX: fputc('x', fp); break;
		case FT_DEC: fputc('d', fp); break;
		case FT_OCT: fputc('o', fp); break;
		case FT_BIN: fputc('b', fp); break;
		case FT_STR: fputc('a', fp); break;
		case FT_STRLINE: fputc('A', fp); break;
		case FT_VSTR: fputc('s', fp); break;
		case FT_VSTR2: fputc('S', fp); break;
		case FT_SPCHAR: fputc('z', fp); break;
		case FT_VHEX: fputc('v', fp); break;
		case FT_VHEX2: fputc('V', fp); break;
		case FT_FLOAT: fputc('f', fp); break;
		case FT_REL: fputc('y', fp); break;
		case FT_INSTR1: fputc('i', fp); break;
		case FT_RPL1: fputc('p', fp); break;
		case FT_GRBROW: fputc('g', fp); break;
		default: fputc('?', fp); break;
		}
	  
	  if(ent->width != 1)
	    fprintf(fp, "%d", ent->width);

	  if(ent < fmt->fmtspec + fmt->nformats - 1)
	    fputc(',', fp);
	}
    }

  fputc(termc, fp);
}


/* Create reference array */
static void create_fmtref(cmpfunc)
  int (*cmpfunc)();
{
     struct symbol **fmtrefptr, *fmtptr;
     
     if(fmtref)
	  free(fmtref);
     fmtref =(struct symbol **) malloc(sizeof(struct symbol *) * (nformats+1));
     for(fmtrefptr = fmtref, fmtptr = fmtroot; fmtptr; fmtptr = fmtptr->link)
	  *fmtrefptr++ = fmtptr;
     *fmtrefptr++ = NULL;
     /* Sort the reference array in ascending value order */
     qsort((char *) fmtref, nformats, sizeof(struct symbol *), cmpfunc); 
}


/* Load formats. */
void load_formats(fmtfname)
  char *fmtfname;
{
  FILE *fmtfile;
  char lbuf[132], *cp;
  struct symbol *fmttemp, **fmtrefptr, *fmtptr;
  int seq;

  if(!(fmtfile = fopen(fmtfname, "r")))
   {
	error("Cannot open formats file: %s", fmtfname);
	exit(1);
   }
  
  for(; !feof(fmtfile); )
   {
	fgets(lbuf, sizeof lbuf, fmtfile);
	if(feof(fmtfile))
	     break;
	if(!(fmttemp = (struct symbol *) malloc(sizeof(struct symbol))))
	 {
	      error("can't alloc format slot");
	      exit(1);
	 }
	fmttemp->link = fmtroot;
	fmtroot = fmttemp;
	cp = lbuf;
	hexstrtoi(&cp, &fmttemp->val);
	if(*cp)
	     cp++;
	if(!*cp || !(fmttemp->form = formatp(&cp, '\n')))
	 {
	      fmtroot = fmttemp->link;
	      free(fmttemp);
	      continue;
	 }
	fmttemp->seq = fmtseq;
	fmttemp->type = 0;
	nformats++;
   }
  fclose(fmtfile);
  create_fmtref(symcmp);
}

/* Add auto formats to formats */
void add_autos_to_fmttab()
{
     nformats += nautoformats;
     nautoformats = 0;
     create_fmtref(symcmp);
}


/* Look up format by value.
 */
struct symbol *fmt(val)
  int val;
{
  int bot, top, cent;


  top = nformats-1;
  bot = 0;
  cent = nformats >> 1;

  for(; top >= bot; cent = bot + ((top-bot) >> 1))
    if(val > fmtref[cent]->val)
      bot = cent+1;
    else
      if(val < fmtref[cent]->val)
	top = cent-1;
      else
	return(fmtref[cent]);
  
  return(NULL);
}


/* Add auto format */
void add_auto_format(addr, type)
  int addr, type;
{
     register struct symbol *fmttemp;
     
     
     /* Ignore if we already have an auto format there */
     /* if((fmttemp = fmt(addr)) && (fmttemp->type & T_FMT_AUTO)) return; */

     /* Changed it to disallow overwriting any existing format */
     /* because it cause overriding info in .formats */

     if(fmttemp = fmt(addr)) return;

     if(!fmttemp)
      {
	   if(!(fmttemp = (struct symbol *) malloc(sizeof(struct symbol))))
	    {
		 error("can't alloc auto format slot");
		 exit(1);
	    }
	   
	   fmttemp->link = fmtroot;
	   fmtroot = fmttemp;
	   nautoformats++;
      }
     else
	  if(fmttemp->form)
	       fmt_free(fmttemp->form);
     
     fmttemp->seq = (fmtroot && fmtroot->val == addr ? fmtroot->seq + 1 : 0);
     fmttemp->type = T_FMT_AUTO | type;
     fmttemp->val = addr;
     fmttemp->form = NULL;
     
     format_added = TRUE;
}


/* Print auto formats to file */
void print_format_file(fmtfname)
char *fmtfname;
{
     register struct symbol **fmtrefptr;
     FILE *fmtfile;
     
     if(!fmtref)
	  return;
     if(!(fmtfile = fopen(fmtfname, "w")))
      {
	   perror(fmtfname);
	   return;
      }
     
     for(fmtrefptr = fmtref; *fmtrefptr; fmtrefptr++)
	  if((*fmtrefptr)->type & T_FMT_AUTO)
	   {
		struct symbol *f = *fmtrefptr;
		int tag;
		
		tag = f->val;
		fprintf(fmtfile, "%X:", tag);
		switch(f->type & T_FMT_TYPE)
		 {
		 case T_FMT_RPL:  fputc('r', fmtfile); break;
		 case T_FMT_CODE: fputc('c', fmtfile); break;
		 case T_FMT_HASH: fputc('h', fmtfile); break;
		 case T_FMT_LINK: fputc('l', fmtfile); break;
		 default: fputc('?', fmtfile); break;
		 }
		fputc('\n', fmtfile);
		
		fmtrefptr++;
		while((*fmtrefptr)->type & T_FMT_AUTO &&
		      (*fmtrefptr)->val == tag)
		     fmtrefptr++;
	   }
     fclose(fmtfile);
}






