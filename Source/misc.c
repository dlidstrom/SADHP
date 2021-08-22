/* misc.c -- SAD context and some common functions.
   
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
#include <signal.h>
#include "globals.h"

/* Initialize */
void init()
{
     char *cp, i;
     
     for(cp = hextoi_map; cp < hextoi_map + sizeof hextoi_map; *cp++ = (char)-1);
     for(i = 0, cp = hexmap; *cp; hextoi_map[*cp++] = i++);
     for(i = 0xa, cp = "ABCDEF"; *cp; hextoi_map[*cp++] = i++);
     
     symroot = comroot = NULL;
     comref = symref = NULL;
}

/* ********************************************************************** */
/* *               Misc utilities                                       * */
/* ********************************************************************** */

/* Set new PC */
void set_pc(addr)
    int addr;
{
     if(fseek(core_file, (long) (addr-baseaddr), 0) < 0)
      {
	   error("address %X not in corefile.\n", addr);
	   saderror();
      }
     pc = addr;
}


/* Try to determine PC. */
int determine_pc(fp)
    FILE *fp;
{
     long oldpos = ftell(fp);
     char lbuf[132], *lp;
     int ahead_pc;
     
     while(!feof(fp))
      {
           fgets(lbuf, sizeof lbuf, fp);
           if(feof(fp))
                break;
           
           lp = lbuf;
           if(IS_HEX(*lp) && hexstrtoi(&lp, &ahead_pc))
	    {
		 fseek(fp, oldpos, 0);
		 fgets(lbuf, sizeof lbuf, fp);
		 fseek(fp, oldpos, 0);

		 return(ahead_pc);
	   }
      }
     
     fseek(fp, oldpos, 0);
     fgets(lbuf, sizeof lbuf, fp);
     fseek(fp, oldpos, 0);
     
     warning("Unable to determine location, assuming %05X:\n%s", pc, lbuf);
     
     return(pc);
}



/* Fetch unibbles from location without updating pcstr */
int fetch_unibbles(loc, len)
    int loc, len;
{
     int nibs, old_pc;
     
     old_pc=pc;
     set_pc(loc);
     nibs=get_pcunibbles(len);
     set_pc(old_pc);
     return(nibs);
}


/* Fetch nibbles from location without updating pcstr */
int fetch_nibbles(loc, len)
    int loc, len;
{
     int nibs, old_pc;
     
     old_pc=pc;
     set_pc(loc);
     nibs=get_pcnibbles(len);
     set_pc(old_pc);
     return(nibs);
}


/* Fetch nibbles as unsigned integer */
get_unibbles(n)
    register n;
{
     register data = 0;
     char *cp, rbuf[64], *pt;
     
     if(!n)
	  return;
     if(n == 1)
	  rbuf[0] = fgetc(core_file);
     else
	  fread(rbuf, 1, n, core_file);
     pt = rbuf+n;
     pc += n;
     cp = (l_codep += n);
     while(n--)
      {
	   register nib = *--pt & 0xf;
	   *--cp = I_TO_HEX(nib);
	   data <<= 4;
	   data |= nib;
      }
     return(data);
}

/* Fetch nibbles as unsigned integer, do not update pc string */
get_pcunibbles(n)
    register n;
{
     register data = 0;
     char rbuf[64], *pt;
     
     if(!n)
	  return;
     if(n == 1)
	  rbuf[0] = fgetc(core_file);
     else
	  fread(rbuf, 1, n, core_file);
     pt = rbuf+n;
     pc += n;
     while(n--)
      {
	   register nib = *--pt & 0xf;
	   data <<= 4;
	   data |= nib;
      }
     return(data);
}


/* Fetch nibbles as integer and extend sign */
get_nibbles(n)
    register n;
{
     register data = 0;
     char *cp, rbuf[64], *pt;
     int nib;
     
     if(n <= 0)
	  return;
     if(n == 1)
	  rbuf[0] = fgetc(core_file);
     else
	  fread(rbuf, 1, n, core_file);
     pt = rbuf+n;
     pc += n;
     cp = (l_codep += n);
     nib = *--pt & 0xf;
     *--cp = I_TO_HEX(nib);
     if(nib & 010)
	  data = (-1 << 4);
     for(data |= nib; --n; )
      {
	   nib = *--pt & 0xf;
	   *--cp = I_TO_HEX(nib);
	   data <<= 4;
	   data |= nib;
      }
     return(data);
}

/* Fetch nibbles as integer and extend sign, do not update PC string */
get_pcnibbles(n)
    register n;
{
     register data = 0;
     char rbuf[64], *pt;
     int nib;
     
     if(n <= 0)
	  return;
     if(n == 1)
	  rbuf[0] = fgetc(core_file);
     else
	  fread(rbuf, 1, n, core_file);
     pt = rbuf+n;
     pc += n;
     nib = *--pt & 0xf;
     if(nib & 010)
	  data = (-1 << 4);
     for(data |= nib; --n; )
      {
	   nib = *--pt & 0xf;
	   data <<= 4;
	   data |= nib;
      }
     return(data);
}

/* Create hex string from pc */
char *make_hexstr(wid)
    int wid;
{
     static char hexbuf[5000], *cp;
     int slen,c;
     
     if(wid>4900)
      {
	   error("Too long hex string at %X (Len %d).\n", pc,wid);
	   saderror();
      }
     
     for(cp = hexbuf, slen = wid; slen > 0; slen--)
      {
	   c = get_unibbles(1);
	   sprintf(cp,"%01X",c);
	   cp++;
      }
     *cp = '\0';
     return (hexbuf);
}

/* Create hex string from pc */
char *make_hexstr_rev(wid)
    int wid;
{
     static char hexbuf[5000], *cp;
     int slen,c;
     
     if(wid>4900)
      {
	   error("Too long hex string at %X (Len %d).\n", pc,wid);
	   saderror();
      }

     /* Skip to end */
     cp = hexbuf ;
     cp += wid;
     *cp-- = '\0';

     for(slen = wid; slen > 0; slen--)
      {
	   c = get_unibbles(1);
	   *cp-- = (c <= 9) ? ('0'+c) : ('A'+c-10);
      }
     return (hexbuf);
}

/* Calculate percentage of good ascii chars in stream */
int ascii_depth(wid)
    int wid;
{
     int i,c,num;
     
     for(i=0, num=0; i < wid ; i++)
      {
	   c = get_pcunibbles(2);
	   if(IS_GOOD_CHAR(c)) num++;
      }
     set_pc(pc-wid*2);
     return (num*100/wid);
}

/* Create short ascii string, no PC update */
char *make_asc(wid)
    int wid;
{
     static char ascbuf[20], *cp;
     int slen,c;
     
     for(cp = ascbuf, slen = wid; slen > 0; slen--)
      {
	   c = get_pcunibbles(2);

	   if( (c < ' ') | (c >= 127) )
	    {
		 switch(c)
		  {
		  case '\n': sprintf(cp,"\\n"); break;
		  case '\t': sprintf(cp,"\\t"); break;
		  default:   sprintf(cp, "\\%02X", c); break;
		  } 
		 cp += strlen(cp);
	    }
	   else
		*cp++ = c;
      }
     *cp = '\0';
     return (ascbuf);
}


/* Create ascii string from pc */
char *make_ascstr(wid, special, spacep)
    int wid, special, spacep;
{
     static char ascbuf[1000], *cp;
     int slen,c;
     
     if(wid>999)
      {
	   error("Too long string at %X (Len %d).\n", pc,wid);
	   saderror();
      }
     for(cp = ascbuf, slen = wid; slen > 0; slen--)
      {
	   if(special)
	    {
		 c = get_pcunibbles(2);
		 if(c>SPC_CHARS)
		      c=' ';
		 else
		      c = spc_chars[c];
	    }
	   else
		c = get_unibbles(2);
	   
	   if( (c < ' ' || c >= 127) | (!spacep & (c == ' ')))
	    {
		 switch(c)
		  {
		  case '\n': sprintf(cp,"\\n"); break;
		  default:   sprintf(cp, "\\%02X", c); break;
		  }
		 cp += strlen(cp);
	    }
	   else
		*cp++ = c;
      }
     *cp = '\0';
     return (ascbuf);
}

/* Create ascii line from pc */
char *make_strline(wid, special)
    int wid, special;
{
     static char ascbuf[1000], *cp;
     int slen,c;
     
     if(wid>999)
      {
	   error("Too long string at %X (Len %d).\n", pc,wid);
	   saderror();
      }
     for(cp = ascbuf, slen = wid; slen > 0; slen--)
      {
	   if(special)
	    {
		 c = get_pcunibbles(2);
		 if(c>SPC_CHARS)
		      c=' ';
		 else
		      c = spc_chars[c];
	    }
	   else
		c = get_unibbles(2);

	   if( (c < ' ') || (c>=127) )
	    {
		 switch(c)
		  {
		  case 0xA: sprintf(cp,"\\n"); break;
		  default: sprintf(cp, "\\%02X", c); break;
		  }
		 cp += strlen(cp);
	    }
	   else
		*cp++ = c;
	   /* End of ascii line? */
	   if( (c=='\n') | (c=='\0'))
		slen = 0;
      }
     *cp = '\0';
     return (ascbuf);
}


/* Draw grob row from pc */
char *make_grobstr(wid)
    int wid;
{
     static char grbdat[2500], *cp;
     int col, grob_nib, bit;
     
     cp = grbdat;
     for(col=0; col < wid ; col++ )
      {
	   grob_nib=get_pcunibbles(1);
	   for(bit=0; bit<4; bit++)
		if(grob_nib&(1<<bit))
		     *cp++ = '*';
		else
		     *cp++ = ' ';
      }
     *cp='\0';
     return(grbdat);
}

/* ********************************************************************** */
/* *               Miscellaneous data handling                          * */
/* ********************************************************************** */

/* Convert address to hex or decimal if small enough */
char *addrtohex(addr)
    int addr;
{
     static char addrbuf[32];
     
     if(abs(addr) < 0xa)
	  sprintf(addrbuf, "%d", addr);
     else
	  sprintf(addrbuf, "#%X", addr);
     
     return(addrbuf);
}


/* Convert hex string to integer, leave pointer at first nonhex char. */
hexstrtoi(cpp, ip)
    register char **cpp;
    int *ip;
{
     register data;
     char *save;
     
     if(!cpp || !*cpp)
	  return(0);
     save = *cpp;
     
     /* Skip optional initial hash sign */
     if(**cpp == '#')
	  (*cpp)++;
     
     for(data = 0; IS_HEX((char)toupper((int)**cpp)); (*cpp)++)
      {
	   data <<= 4;
	   data |= HEX_TO_I(toupper((int)**cpp));
      }
     
     if(IS_INNER_SYM(**cpp))
      {
	   *cpp = save;
	   return(FALSE);
      }
     *ip = data;
     return(TRUE);
}


/* Convert any (< 10) base string to integer,
 * leave pointer at first nonhex char.
 */
anystrtoi(cpp, ip, base)
    register char **cpp;
    int *ip, base;
{
     register data;
     char *save, basemax;
     
     if(!cpp || !*cpp)
	  return(FALSE);
     
     save = *cpp;
     
     basemax = base + '0' - 1;
     for(data = 0; (**cpp >= '0' && **cpp <= basemax); (*cpp)++)
      {
	   data *= base;
	   data += **cpp-'0';
      }
     
     if(IS_INNER_SYM(**cpp))
      {
	   *cpp = save;
	   return(FALSE);
      }
     
     *ip = data;
     return(TRUE);
}


/* Convert integer string to integer,
 * leave pointer at first nonhex char.
 */
intstrtoi(cpp, ip)
    register char **cpp;
    int *ip;
{
     if(!cpp || !*cpp)
	  return(0);
     
     if(**cpp == '#')
	  if((*cpp)[1] == 'o')
	   {
		(*cpp) += 2;
		return(anystrtoi(cpp, ip, 8));
	   }
	  else
	       return(hexstrtoi(cpp, ip));
     return(anystrtoi(cpp, ip, 10));
}


/* Convert real to string */
char *realtos(rp, bp)
    hp_real *rp;
    char *bp;
{
     char *bp0 = bp;
     int realexp;
     
     sprintf(bp, "%s%X.%08X%03X",
	     (rp->s >= 5 ? "-" : ""),
	     rp->m1, rp->m, rp->mr);
     
     bp += strlen(bp) - 1;
     
     while(*bp == '0') bp--;
     
     if(*bp == '.') bp--;
     *++bp = '\0';
     
     realexp = hxstoreal(rp->x);
     
     if(rp->x)
	  sprintf(bp, "E%s%d",
		  (realexp >= 500 ? "-" : ""),
		  (realexp < 500 ? realexp : 999 - realexp + 1));
     
     return(bp0);
}


/* Convert long real to string */
char *longrealtos(lrp, bp)
    hp_longreal *lrp;
    char *bp;
{
     char *bp0 = bp;
     int realexp;
     
     sprintf(bp, "%s%X.%08X%06X",
	     (lrp->s >= 5 ? "-" : ""),
	     lrp->m1, lrp->m, lrp->mr);
     
     bp += strlen(bp) - 1;
     
     while(*bp == '0') bp--;
     if(*bp == '.') bp--;
     *++bp = '\0';
     
     realexp = hxstoreal(lrp->x);
     
     if(lrp->x)
	  sprintf(bp, "E%s%d",
		  (realexp >= 50000 ? "-" : ""),
		  (realexp < 50000 ? realexp : 99999 - realexp + 1));
     return(bp0);
}

/* Used to avoid the letter 'A' when printing exponent after calculations */
int hxstoreal(hxs)
    int hxs;
{
     int num = 0, counter = 1;
     while (hxs != 0)
      {
	   num += (hxs&0xF)*counter;
	   counter *= 10;
	   hxs >>= 4;
      }
     return num;
}

/* Convert integer to bit mask */
char *itobmask(i)
    register unsigned i;
{
     register bitno;
     static char bitbuf[3*32+5];
     register char *bp = bitbuf;
     
     *bp++ = '[';
     
     for(bitno = 0; i; i >>= 1, bitno++)
	  if(i & 1)
	   {
		sprintf(bp, "%d", bitno);
		bp += strlen(bp);
		
		if(i > 1)
		     *bp++ = ',';
	   }
     
     *bp++ = ']';
     *bp = '\0';
     
     return(bitbuf);
}


/* Duplicate string */
char *strdup(s)
    char *s;
{
     register char *tmp;
     extern char *strcpy();
     
     if(!s)
	  return("");
     if(!(tmp = (char *) malloc(strlen(s)+1)))
      {
	   perror("malloc");
	   exit(1);
      }
     return(strcpy(tmp, s));
}


/* ********************************************************************** */
/* *               Comments handling                                    * */
/* ********************************************************************** */

/* Compare two comments by value, type, and sequence for ascending sort. */
int comcmp(c1, c2)
    struct symbol **c1, **c2;
{
     /* First on value */
     if((*c1)->val > (*c2)->val)
	  return(1);
     if((*c1)->val < (*c2)->val)
	  return(-1);
     /* Then on type */
     if(((*c1)->type & T_COM_TYPE) > ((*c2)->type & T_COM_TYPE))
	  return(-1);
     if(((*c1)->type & T_COM_TYPE) < ((*c2)->type & T_COM_TYPE))
	  return(1);
     /* Then on sequence */
     if((*c1)->seq > (*c2)->seq)
	  return(1);
     if((*c1)->seq < (*c2)->seq)
	  return(-1);
     return(0);
}

/* Load comments. */
void load_comments(comfname)
    char *comfname;
{
     FILE *comfile;
     char lbuf[132], *cp;
     struct symbol *comtemp, **comrefptr, *comptr;
     int seqaddr, seq;
     
     if(!(comfile = fopen(comfname, "r")))
	  return;
     
     comroot = NULL;
     for(ncomments = seqaddr = seq = 0; !feof(comfile); )
      {
	   fgets(lbuf, sizeof lbuf, comfile);
	   if(feof(comfile))
		break;
	   
	   lbuf[strlen(lbuf)-1] = '\0';
	   comtemp = (struct symbol *) malloc(sizeof(struct symbol));
	   comtemp->link = comroot;
	   comroot = comtemp;
	   
	   cp = lbuf;
	   hexstrtoi(&cp, &comtemp->val);
	   
	   if((!*cp) || (*cp=='*'))
	    {
		 comroot = comtemp->link;
		 free(comtemp);
		 continue;
	    }
	   
	   comtemp->seq = ++seq;
	   comtemp->type = (*cp++ == '=' ? T_COM_MAJOR : T_COM_MINOR);
	   comtemp->id = strdup(cp);
	   ncomments++;
      }
     
     fclose(comfile);
     
     /* Create reference array, end with NULL guard */
     comref=(struct symbol **) malloc(sizeof(struct symbol *) * (ncomments+1));
     
     for(comrefptr = comref, comptr = comroot; comptr; comptr = comptr->link)
	  *comrefptr++ = comptr;
     
     *comrefptr++ = NULL;
     
     
     /* Sort the reference array in ascending value order */
     qsort((char *) comref, ncomments, sizeof(struct symbol *), comcmp);
}


/* Add to comment field */
void comment(ctrl, a1, a2, a3, a4)
    char *ctrl, *a1, *a2, *a3, *a4;
{
     if(!opt_comments)
	  return;
     *l_commentp++ = ' ';
     sprintf(l_commentp, ctrl, a1, a2, a3, a4);
     l_commentp += strlen(l_commentp);
}


/* Add to comment field */
void force_comment(ctrl, a1, a2, a3, a4)
    char *ctrl, *a1, *a2, *a3, *a4;
{
     /*
      * if(!opt_source & !opt_comments)
      * return;
      */
     *l_commentp++ = ' ';
     sprintf(l_commentp, ctrl, a1, a2, a3, a4);
     l_commentp += strlen(l_commentp);
}



/* ********************************************************************** */
/* *               Symbols handling                                     * */
/* ********************************************************************** */


/* Compare two symbols by value for ascending sort. */
symcmp(s1, s2)
    struct symbol **s1, **s2;
{
     if((*s1)->val > (*s2)->val)
	  return(1);
     if((*s1)->val < (*s2)->val)
	  return(-1);
     if((*s1)->seq > (*s2)->seq)
	  return(1);
     if((*s1)->seq < (*s2)->seq)
	  return(-1);
     return(0);
}


/* Create symbol reference array */
void create_symref(compfunc)
    int (*compfunc)();  
{
     struct symbol **symrefptr, *symptr;
     
     if(symref)
	  free(symref);
     
     /* Create reference array, end with NULL guard */
     symref =(struct symbol **) malloc(sizeof(struct symbol *) * (nsymbols+1));
     
     for(symrefptr = symref, symptr = symroot; symptr; symptr = symptr->link)
	  *symrefptr++ = symptr;
     
     *symrefptr++ = NULL;
     
     /* Sort the reference array in ascending order by name */
     qsort((char *) symref, nsymbols, sizeof(struct symbol *), compfunc);
}

/* Load symbols */
void load_symbols(compfunc)
    int (*compfunc)();
{
     
     FILE *symfile;
     char lbuf[132], *cp;
     struct symbol *symtemp, **symrefptr, *symptr;
     
     nsymbols=0;

     if(opt_gx)
	  symfile = fopen(SAD_SYMBOLSGX, "r");
     else
	  symfile = fopen(SAD_SYMBOLS, "r");
     if(symfile)
	  for(; !feof(symfile); )
	   {
		
		fgets(lbuf, sizeof lbuf, symfile);
		if(feof(symfile))
		     break;
		lbuf[strlen(lbuf)-1] = '\0';
		symtemp = (struct symbol *) malloc(sizeof(struct symbol));
		symtemp->link = symroot;
		symroot = symtemp;
		cp = lbuf;
		hexstrtoi(&cp, &symtemp->val);
		switch(*cp++)
		 {
		 case '\0':
		      symroot = symtemp->link;
		      free(symtemp);
		      continue;
		 case '=':
		      symtemp->type = T_SYM_HP;
		      break;
		 case ',':
		      if(opt_hp)
		       {
			    symroot = symtemp->link;
			    free(symtemp);
			    continue;
		       }
		      else
			   symtemp->type = T_SYM_REVDEP;
		      break;
		 case ':':
		 default:
		      if(opt_hp)
		       {
			    symroot = symtemp->link;
			    free(symtemp);
			    continue;
		       }
		      else
			   symtemp->type = T_SYM_NOHP;
		      break;
		 }
		symtemp->seq = nsymbols ;
		symtemp->ref = 0;
		symtemp->id = strdup(cp);
		symtemp->xrefhead = symtemp->xreftail = NULL;
		nsymbols++;
	   }
     fclose(symfile);
     
     /* Couldn't get this damn thing into a subroutine */
     
     switch(baseaddr)
      {
      case 0x0:
	   create_symref(compfunc);
	   return;
      case 0x80000:
	   if(!(symfile = fopen(SAD_SYMBOLS1, "r")))
	    {
		 if(nsymbols)
		      create_symref(compfunc);
		 return;
	    }
	   break;
      case 0xC0000:
	   if(!(symfile = fopen(SAD_SYMBOLS2, "r")))
	    {
		 if(nsymbols)
		      create_symref(compfunc);
		 return;
	    }
	   break;
      }
     
     for(; !feof(symfile); )
      {
	   fgets(lbuf, sizeof lbuf, symfile);
	   if(feof(symfile))
		break;
	   
	   lbuf[strlen(lbuf)-1] = '\0';
	   symtemp = (struct symbol *) malloc(sizeof(struct symbol));
	   symtemp->link = symroot;
	   symroot = symtemp;
	   cp = lbuf;
	   hexstrtoi(&cp, &symtemp->val);
	   switch(*cp++)
	    {
	    case '\0':
		 symroot = symtemp->link;
		 free(symtemp);
		 continue;
	    case '=':
		 symtemp->type = T_SYM_HP;
		 break;
	    case ',':
		 symtemp->type = T_SYM_REVDEP;
		 break;
	    case ':':
	    default:
		 symtemp->type = T_SYM_NOHP ; /* Was T_SYM_LNAME */
		 break;
	    }
	   symtemp->seq = nsymbols;
	   symtemp->ref = 0;
	   symtemp->id = strdup(cp);
	   symtemp->xrefhead = symtemp->xreftail = NULL;
	   nsymbols++;
      }
     fclose(symfile);
     create_symref(compfunc);
}


/* Add local name */
void add_local_name(val, lname, type)
    int val, type;
    char *lname;
{
     register struct symbol *symtemp;
     
     symtemp = (struct symbol *) malloc(sizeof(struct symbol));
     symtemp->link = symroot;
     symroot = symtemp;

     nlocals++;
     
     symtemp->val = val;
     symtemp->type = type;
     symtemp->seq = nsymbols + nlocals;
     symtemp->ref = 0;
     symtemp->id = strdup(lname);
     symtemp->xrefhead = symtemp->xreftail = NULL;
     
}


/* Add local symbol */
void add_local_symbol(val, type)
    int val, type;
{
     register struct symbol *symtemp;
     
     symtemp = (struct symbol *) malloc(sizeof(struct symbol));
     symtemp->link = symroot;
     symroot = symtemp;

     nlocals++;
     
     symtemp->val = val;
     symtemp->type = type | T_SYM_LOCAL;
     symtemp->seq = nsymbols + nlocals;
     symtemp->ref = 0;
     symtemp->id = NULL;
     symtemp->xrefhead = symtemp->xreftail = NULL;
     
}

/* Add local symbols to table */
void add_locals_to_symtab(compfunc)
    int (*compfunc)();
{
     nsymbols += nlocals;
     create_symref(compfunc);
}


/* Add node to xref list */
static void append_xref(sym, xrefval)
    struct symbol *sym;
    int xrefval;
{
     register struct xrefaddr
	  *xrefnode = (struct xrefaddr *) malloc(sizeof(struct xrefaddr));
     
     if(!xrefnode)
      {
	   perror("malloc");
	   return;
      }
     
     xrefnode->val = xrefval;
     xrefnode->link= NULL;
     
     if(!sym->xrefhead)
	  sym->xrefhead = sym->xreftail = xrefnode;
     else
      {
	   sym->xreftail->link = xrefnode;
	   sym->xreftail = xrefnode;
      }
}


/* Return ID of local symbol. */
char *local_id(symp)
    struct symbol *symp;
{
     static char sname[132];
     if(symp->id == NULL)
	  sprintf(sname, "L_%05X", symp->val);
     else
	  sprintf(sname, "%s", symp->id);
     return(sname);
}

/* Return name from symb table according to type / format */
static char *symb_name(loc,maxoffs)
    int loc,maxoffs;
{
     static char symbname[132];
     
     int islocal = (symref[loc]->type & T_SYM_LOCAL) != 0;
     int islname = (symref[loc]->type & T_SYM_LNAME) != 0;
     
     if(islocal | islname)
	  if(maxoffs == NORELRPL)
	       sprintf(symbname, "PTR %s",
		       (islocal) ? local_id(symref[loc]) : symref[loc]->id ); 
	  else
	       sprintf(symbname, "%s",
		       (islocal) ? local_id(symref[loc]) : symref[loc]->id ); 
     else
	  /* Global label */
	  if(maxoffs == NORELRPL)
	       sprintf(symbname, "%s", symref[loc]->id); 
	  else
	       sprintf(symbname, "=%s", symref[loc]->id); 
     
     return (symbname);
}	  

/* Map address to symbol with or w/o offset, if feasible */
/* If address is small don't use labels */
char *cksymbolic(addr, maxoffs, codep)
    int addr, maxoffs, codep;
{
     extern char *symbolic();
     
     static char symbuf2[132];
     
     if (addr<0x100)
	  strcpy(symbuf2,addrtohex(addr));
     else
	  strcpy(symbuf2, symbolic(addr,maxoffs,codep));
     return(symbuf2);
}


/* Map address to symbol with or w/o offset, if feasible */
char *symbolic(addr, maxoffs, codep)
    int addr, maxoffs, codep;
{
     static char symbuf[132];
     int bot, top, cent;
     struct symbol *closest;
     
     if(!symref || !symroot || !nsymbols)
      {
	   strcpy(symbuf,addrtohex(addr));
	   return(symbuf);
      }
     
     /* Look up symbol */
     top = nsymbols-1;
     bot = 0;
     cent = nsymbols >> 1;

     
     for(; top > bot+1; cent = bot + ((top-bot) >> 1))
	  if(addr > symref[cent]->val)
	       bot = cent;
	  else if (addr < symref[cent]->val)
	       top = cent;
	  else
	   {
		int islocal;
		
		/* Scanning for following declarations */
		
		while((cent<nsymbols-1) && (symref[cent+1]->val==addr))
		     cent++;
		
		islocal = (symref[cent]->type & T_SYM_LOCAL) != 0;

		/* Now backing up if we could find a global label */
		/* or a named local */
		
		while(islocal&&(cent>0)&&(symref[cent-1]->val==addr))
		 {
		      cent--;
		      islocal = (symref[cent]->type & T_SYM_LOCAL) != 0;
		 }

		/* Now fwd if all we have is local names */
		while(islocal&&(cent<nsymbols-1)&&(symref[cent-1]->val==addr))
		 {
		      cent++;
		      islocal = (symref[cent]->type & T_SYM_LOCAL) != 0;
		      if(islocal)
			   islocal = symref[cent]->id != NULL;
		 }
		islocal = (symref[cent]->type & T_SYM_LOCAL) != 0;
		
		if((opt_symbols) | (addr>0xFFFFF))
		 {
		      strcpy(symbuf,symb_name(cent, maxoffs));
		      if((!islocal) && (addr<=0xFFFFF))
			   if( (symref[cent]->type & T_SYM_TYPE) == T_SYM_HP )
				comment(addrtohex(addr));
			   else if ((symref[cent]->type & T_SYM_TYPE) == T_SYM_NOHP )
				comment("!%X",addr);
			   else if ((symref[cent]->type & T_SYM_TYPE) == T_SYM_REVDEP )
				comment("!!%X",addr);
		 }
		else
		 {
		      if(codep)
			   sprintf(symbuf,"%s",addrtohex(addr));
		      else
			   sprintf(symbuf,"PTR %X",addr);
		      if((symref[cent]->type & T_SYM_TYPE) == T_SYM_HP)
			   comment("%s", symb_name(cent, maxoffs));
		      else if((symref[cent]->type & T_SYM_TYPE) == T_SYM_NOHP)
			   comment("(%s)", symb_name(cent, maxoffs));
		      else if((symref[cent]->type & T_SYM_TYPE)==T_SYM_REVDEP)
			   comment("((%s))", symb_name(cent, maxoffs));
		 }
		if(opt_xref && pass == PASS2)
		     append_xref(symref[cent], org_pc);
		symref[cent]->ref++;
		return(symbuf);
	   }
    

     /* Now, we use bot if it's close enough, otherwise top if it's close
      * enough. This since we always prefer + offsets to - offsets.
      */

     if((abs(addr - symref[bot]->val) < maxoffs) &&
	(addr <= symref[top]->val))
	  closest = symref[bot];
     else if(abs(symref[top]->val - addr) < maxoffs)
	  closest = symref[top];
     else
      {
	   if(opt_locals && (pass == PASS1) &&
	      (addr >= opt_startpt) && (addr <= opt_endpt))
	    {
		 add_local_symbol(addr, T_SYM_NOHP);
		 if(maxoffs == NORELRPL)
		      sprintf(symbuf, "PTR %s", local_id(symroot));
		 else
		      strcpy(symbuf, local_id(symroot));
	    }
	   else
	    {
		 if(maxoffs == NORELRPL)
		      sprintf(symbuf, "PTR %X", addr);
		 else
		      strcpy(symbuf, addrtohex(addr));
	    }
	   return(symbuf);
      }
     if(addr == closest->val)
      { 
	   int islocal = ((closest->type & T_SYM_LOCAL) != 0);
	   
	   if(opt_symbols)
	    {
		 strcpy(symbuf, (islocal ? local_id(closest) : closest->id));
		 if(!islocal)
		      comment(addrtohex(addr));
	    }
	   else
	    {
		 sprintf(symbuf,"CON(5)  %s",addrtohex(addr));
		 comment("%s", (islocal ? local_id(closest) : closest->id));
	    }
	   
	   if(opt_xref && pass == PASS2)
		append_xref(closest, org_pc);
	   
	   closest->ref++;
	   return(symbuf);
      }
     else
	  /* Build expression in local buffer, and recurse for offset */
      {
	   char lsymbuf[132];
	   
	   if(opt_locals && (pass == PASS1) &&
	      (addr >= opt_startpt) && (addr <= opt_endpt))
	    {
		 add_local_symbol(addr, T_SYM_NOHP);
		 strcpy(symbuf, local_id(symroot));
	    }
	   else
	    {
		 closest->ref++;
		 if(closest->type & T_SYM_LOCAL)
		      sprintf(lsymbuf, "%s%c%s",
			      local_id(closest),
			      (addr > closest->val ? '+' : '-'),
			      cksymbolic(abs(closest->val -addr),NOREL,codep));
		 else
		      sprintf(lsymbuf, "(=%s)%c%s",
			      closest->id,
			      (addr > closest->val ? '+' : '-'),
			      cksymbolic(abs(closest->val -addr),NOREL,codep));
		 
		 
		 if(opt_symbols)
		  {
		       strcpy(symbuf, lsymbuf);
		       if(!(closest->type & T_SYM_LOCAL))
			    comment(addrtohex(addr));
		  }
		 else
		  {
		       if(codep)
			    sprintf(symbuf,"%s", addrtohex(addr));
		       else
			    sprintf(symbuf,"PTR %X", addr);
		       comment(lsymbuf);
		  }
	    }
	   if(opt_xref)
		append_xref(closest, org_pc);
      }
     
     return(symbuf);
}
