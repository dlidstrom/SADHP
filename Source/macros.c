/* macros.c -- SAD macros related code.

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
#include "macros.h"
#include "formats.h"
#include "globals.h"


/* Convert any (10 or less) base string to integer,
 * leave pointer at first nonhex char.
 * Adapted from misc.
 */
static mac_anystrtoi(cpp, ip, base)
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


/* Compare two macro patterns */
static maccmp(m1, m2)
register struct symbol **m1, **m2;
{
     if((*m1)->seq > (*m2)->seq)
	  return(1);
      if((*m1)->seq < (*m2)->seq)
	  return(-1);
     if((*m1)->val > (*m2)->val)
	  return(1);
     if((*m1)->val < (*m2)->val)
	  return(-1);
     return(0);
}


/* Load macros. */
void load_macros(macfname)
char *macfname;
{
     FILE *macfile;
     char lbuf[132], *cp;
     struct symbol *mactemp, **macrefptr, *macptr;
     int anint;
     extern char *malloc();
     
     if(!(macfile = fopen(macfname, "r")))
	  return;
     
     macroot = NULL;
     for(nmacros = 0; !feof(macfile); )
      {
	   fgets(lbuf, sizeof lbuf, macfile);
	   if(feof(macfile))
		break;
	   
	   lbuf[strlen(lbuf)-1] = '\0';
	   
	   if(!(mactemp = (struct symbol *) malloc(sizeof(struct symbol))))
	    {
		 error("can't alloc macro pattern slot");
		 exit(1);
	    }
	   
	   mactemp->link = macroot;
	   macroot = mactemp;
	   
	   cp = lbuf;
	   mac_anystrtoi(&cp, &anint, 10); /* Pattern size */
	   
	   mactemp->seq = anint;
	   
	   if(*cp)
		cp++;
	   
	   hexstrtoi(&cp, &mactemp->val); /* Pattern tag */
	   
	   if(*cp)
		cp++;
	   
	   if(!*cp || !(mactemp->id = strdup(cp)))
	    {
		 macroot = mactemp->link;
		 free(mactemp);
		 continue;
	    }
	   
	   mactemp->type = T_MACRO;
	   
	   nmacros++;
      }
     
     fclose(macfile);
     
     
     /* Create reference array, end with NULL guard */
     macref = (struct symbol **) malloc(sizeof(struct symbol *) * (nmacros+1));
     
     for(macrefptr = macref, macptr = macroot; macptr; macptr = macptr->link)
	  *macrefptr++ = macptr;
     
     *macrefptr++ = NULL;
     
     /* Sort the reference array on a) pattern size, and b) pattern tag */
     qsort((char *) macref, nmacros, sizeof(struct symbol *), maccmp);
}


/* Look up macro on pattern size and tag */
struct symbol *macro(size, tag)
int size, tag;
{
     int bot, top, cent;
     struct symbol *closest, pat, *patp;
     
     if(!macref || !macroot || !nmacros)
	  return(NULL);
     
     /* Look up macro */
     top = nmacros-1;
     bot = 0;
     cent = nmacros >> 1;
     
     pat.seq = size;
     pat.val = tag;
     patp = &pat;
     
     for(; top >= bot; cent = bot + ((top-bot) >> 1))
	  switch(maccmp(&macref[cent], &patp))
	   {
	   case -1: bot = cent+1; break;
	   case 1: top = cent-1; break;
	   default: return(macref[cent]);
	   }
     
     return(NULL);
}


/* Expand pattern.
 * If expansion fails, NULL is returned.
 * Otherwise a static buffer is returned.
 */
char *macro_expand(macdef)
struct symbol *macdef;
{

     static char expbuf[10000];
     char *def, *bp, defc;
     int w, *wp, deflt, data, len, adj;
     
     if(!macdef || !macdef->id)
	  return("");
     
     for(bp = expbuf, def = macdef->id; *def; )
	  if(*def != '%')
	       *bp++ = *def++;
	  else
	   {
		def++;
		adj = 0;
		
		if(!mac_anystrtoi(&def, &w, 10))
		     wp = &deflt;
		else
		     wp = &w;
		adj = 0;
		if(*def == ',')
		 {
		      def++;
		      if(!mac_anystrtoi(&def, &adj, 10))
			   adj = 0;
		 }
		
		switch(defc = *def++)
		 {
		 case '%': *bp++ = '%'; break;
		      
		 case 'I':
		 {
		      extern void format_code();
		      format_code(); 
		      break;
		 }
		 case 'i':
		 {
		      extern void format_code();
		      extern org_pc, rpl_new_ind_level;
		      extern struct format *curfmt;
		      
		      deflt = 5;
		      len = get_unibbles(*wp);
		      prevfmt = curfmt;
		      fmtexpire_tag = org_pc + len - adj + 5;
		      rpl_new_ind_level++;
		      
		      format_code();
		      
		      sprintf(bp, "%d", len - adj);
		      bp += strlen(bp);
		      
		      break;
		 }
		      
		 case 'z':
		 case 'Z':
		      deflt = 1;
		      len = *wp;
		      for(len = *wp; len >= 8; len -= 8)
			   get_unibbles(8);
		      if(len > 0)
			   get_unibbles(len);
		      break;
		      
		 case 's':
		      deflt = 2;
		      if((len = (get_unibbles(*wp) - adj) / 2) < 0)
			   len = 0;
		      if(len > 5000)
		       {
			    comment("string too long");
			    return(NULL);
		       }
		      while(len--)
		       {
			    data = get_unibbles(2);
			    if(data < ' ' || data > 127)
			     {
				  sprintf(bp, "\\%02X", data);
				  bp += strlen(bp);
			     }
			    else
				 if(data=='\'' || data=='"' || data=='\\')
				  {
				       *bp++ = '\\';
				       *bp++ = data;
				  }
				 else
				      *bp++ = data;
		       }
		      break;
		      
		 case 'S':
		      deflt = 2;
		      len = get_unibbles(*wp);
		      if(len > 5000 )
		       {
			    comment("string too long");
			    return(NULL);
		       }
		      while(len--)
		       {
			    data = get_unibbles(2);
			    
			    if(data < ' ' || data > 127)
			     {
				  sprintf(bp, "\\%02X", data);
				  bp += strlen(bp);
			     }
			    else
				 if(data=='\'' || data=='"' || data=='\\')
				  {
				       *bp++ = '\\';
				       *bp++ = data;
				  }
				 else
				      *bp++ = data;
		       }
		      break;
		      
		 case 'c':
		 case 'C':
		      deflt = 1;
		      len = *wp;
		      while(len--)
		       {
			    data = get_unibbles(2);
			    if(data < ' ' || data > 127)
			     {
				  sprintf(bp, "\\%02X", data);
				  bp += strlen(bp);
			     }
			    else
				 *bp++ = data;
		       }
		      break;
		      
		 case 'x':
		 case 'X':
		      deflt = 5;
		      sprintf(bp, "%X", get_unibbles(*wp));
		      bp += strlen(bp);
		      break;
		      
		 case 'd':
		 case 'D':
		      deflt = 5;
		      sprintf(bp, "%d", get_unibbles(*wp));
		      bp += strlen(bp);
		      break;
		      
		 case 'o':
		 case 'O':
		      deflt = 5;
		      sprintf(bp, "#o%o", get_unibbles(*wp));
		      bp += strlen(bp);
		      break;
		      
		 case 'b':
		 case 'B':
		 {
		      static char *bin4[] = {
			    "0000", "0001", "0010", "0011",
			    "0100", "0101", "0110", "0111",
			    "1000", "1001", "1010", "1011",
			    "1100", "1101", "1110", "1111" };
	      
		      deflt = 5;
		      *bp++ = '#';
		      *bp++ = 'b';
		      for(len = *wp; len--; )
		       {
			    strcpy(bp, bin4[get_unibbles(1)]);
			    bp += 4;
		       }
		      break;
		 }
		      
		 case 'n':
		 {
		      extern void print_source();
		      print_source();
		      break;
		 }
		 case 'N':
		 {
		      extern mode_single_op;
		      mode_single_op = TRUE;
		      break;
		 }
		 case 'e':
		 {
		      extern mode_fixed_line, fixed_ops, opt_source;
	
		      deflt = 1;
		      if(opt_source)
		       {
			    mode_fixed_line = TRUE;
			    fixed_ops = *wp;
		       }
		      break;
		 }
		 case 'E':
		 {
		      extern opt_source, rpl_comp_level;
		      extern mode_fixed_line, fixed_ops;
		      extern mode_dispatch;
		      extern dispatch_obs, dispatch_level, dispatch_lines;
		    
		      deflt = 1;
		      if(opt_source)
		       {
			    mode_fixed_line = TRUE;
			    mode_dispatch = TRUE ;
			    /* This forces newline afterwards */
			    fixed_ops = 0;
			    dispatch_obs = *wp-1 ;
			    if(adj)
				 dispatch_lines = adj;
			    else
				 dispatch_lines = 0xFFFFF;
			    dispatch_level = rpl_comp_level;
		       }
		      break;
		 }
		      
		 case '-':
		 {
		      extern rpl_ind_level, rpl_new_ind_level, rpl_comp_level;
		      extern void print_source();

		      print_source();
		      deflt = 1;

		      rpl_ind_level -= *wp;
		      rpl_new_ind_level -= *wp;
		      if(rpl_ind_level < 0)
			   rpl_ind_level = 0;
		      if(rpl_new_ind_level < 0)
			   rpl_new_ind_level = 0;
		      if(rpl_ind_level < rpl_comp_level)
			   rpl_ind_level=rpl_comp_level;
		      if(rpl_new_ind_level < rpl_comp_level)
			   rpl_new_ind_level=rpl_comp_level;
		      break;
		 }
		      
		 case '+':
		 {
		      extern rpl_new_ind_level;
		      
		      deflt = 1;
		      rpl_new_ind_level += deflt;
		      break;
		 }
		 case 'w':
		 case 'W':
		 case 'f':
		 case 'F':
		 {
		      hp_real hp_r;
		      
		      hp_r.x = get_unibbles(3);
		      hp_r.mr= get_unibbles(3);
		      hp_r.m = get_unibbles(8);
		      hp_r.m1= get_unibbles(1);
		      hp_r.s = get_unibbles(1);
		      
		      realtos(&hp_r, bp);
		      bp += strlen(bp);
		      break;
		 }

		 case 'l':
		 case 'L':
		 {
		      hp_longreal hp_lr;

		      hp_lr.x = get_unibbles(5);
		      hp_lr.mr= get_unibbles(6);
		      hp_lr.m = get_unibbles(8);
		      hp_lr.m1= get_unibbles(1);
		      hp_lr.s = get_unibbles(1);
		      
		      longrealtos(&hp_lr, bp);
		      bp += strlen(bp);
		      
		      break;
		 }

		 case 'v':
		 case 'V':
		      deflt = 2;
		      len = get_unibbles(*wp)-adj;
		      *bp++ = '#';
		      if(len > 5000)
		       {
			    comment("vector too long");
			    return(NULL);
		       }
		      while(len-- > 0)
			   *bp++ = I_TO_HEX(get_unibbles(1));
		      break;
		      
		 case '=':
		      deflt = 0;
		      if(get_unibbles(*wp) != adj)
			   return(NULL);
		      break;
		      
		 }
	   }
     
     *bp = '\0';
     return(expbuf);
}
