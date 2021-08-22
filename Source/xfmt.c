/* sadfmt.c.c -- SAD format set/display

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
#include "formats.h"

/* Options */
int
     opt_join = FALSE,		/* Join mode, see opt_joinfile */
     opt_erase = FALSE,		/* Erase */
     opt_rewrite = FALSE,	/* Rewrite .formats */
     opt_set = FALSE,		/* Set (as opposed to view) format at addr */
     opt_addr= 0;		/* Address for view/set */

char
     *opt_joinfile,		/* File to join */
     opt_fmt[64];			/* New format */


/* Read formats file; if subst is true,
 * remove all occurences of addr and append new format at end. Print
 * output on outfile.
 * If false, remember format closest and print it on stdout.
 * If opt_erase is true, erase instead of change.
 */
static void process_formats(fmtfname, addr, fmt, outfile)
    char *fmtfname, *fmt;
    int addr;
    FILE *outfile;
{
     FILE *fmtfile;
     char lbuf[132], *cp, closest_fmt[132];
     int closest_addr = -1, addr_field;
     
     
     if(!(fmtfile = fopen(fmtfname, "r")))
	  return;
     
     while(!feof(fmtfile))
      {
	   fgets(lbuf, sizeof lbuf, fmtfile);
	   if(feof(fmtfile))
		break;
	   
	   cp = lbuf;
	   if(!hexstrtoi(&cp, &addr_field) || !*cp || !cp[1])
		continue;
	   if(!opt_set && !opt_erase)
		if(addr_field <= addr && addr_field > closest_addr)
		 {
		      cp++;
		      
		      if(addr_field == addr)
		       {
			    fputs(cp, stdout);
			    return;
		       }
		      
		      closest_addr = addr_field;
		      strcpy(closest_fmt, cp);
		 }
		else ;
	   else
		if(addr_field != addr)
		     fputs(lbuf, outfile);
      }
     
     fclose(fmtfile);
     
     if(!opt_set && !opt_erase)
	  if(closest_addr >= 0)
	       fputs(closest_fmt, stdout);
	  else
	       puts("c");
     else
	  if(!opt_erase)
	       fprintf(outfile, "%05X:%s\n", addr, fmt);
}


/* Join .formats with file. The file must be sorted. */
static void join_formats(infname, outfile)
    char *infname;
    FILE *outfile;
{
     FILE *infile;
     struct symbol **fmt;
     
     init();
     
     /* Load both files. */
     fmtseq = 1;
     load_formats(infname);
     
     fmtseq = 0;
     load_formats(opt_joinfile);
     
     
     /* Loop and print */
     for(fmt = fmtref; fmt && *fmt;)
      {
	   int tag = (*fmt)->val;
	   
	   /* Print entry */
	   fprintf(outfile, "%X:", (*fmt)->val);
	   fmt_print((*fmt)->form, '\n', outfile);
	   
	   /* Sift out old entries at same address */
	   while(*fmt && (*fmt)->val == tag)
		fmt++;
      }
}


/* Print usage */
void usage(argc, argv)
    int argc;
    char **argv;
{
     fprintf(stderr, "usage: %s [-rjd] addr [fmt]\n", *argv);
     exit(1);
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
		 case 'd':		/* Erase (no fmt) */
		      opt_erase = !opt_erase;
		      break;
		 case 'Q':		/* Debug mode on */
		      opt_debug = !opt_debug;
		      break;
		 case 'j':		/* Join */
		      opt_join = !opt_join;
		      break;
		 case 'r':		/* Rewrite .formats */
		      opt_rewrite = !opt_rewrite;
		      break;
		 default:
		      usage(argc, argv);
		 }
      }
     
     if(argc < 2+flagsp-opt_join)
	  usage(argc, argv);
     
     if(opt_set = ((argc == 3+flagsp) && !opt_join && !opt_erase))
	  strcpy(opt_fmt, argv[2+flagsp]);
     
     cp1 = argv[1+flagsp];

     if(opt_set || opt_erase)
      {
	   if(!hexstrtoi(&cp1, &opt_addr))
		usage(argc, argv);
	   else ;
      }
     else if(opt_join)
	  if(argc < 2+flagsp)
	       opt_joinfile = SAD_AUTOFMT;
	  else
	       opt_joinfile = argv[1+flagsp];
     else /* 'opt_view' */
	  if(!hexstrtoi(&cp1, &opt_addr))
	       usage(argc, argv);

     if(opt_debug)
      {
	   fprintf(stderr, "Debug mode enabled.\nThis is SAD%s %s.\n\n",
		   sad_version, *argv);
	   
	   if(opt_join)
		fprintf(stderr, "Joining with %s\n", opt_joinfile);
      }
}  


main(argc, argv)
    int argc;
    char **argv;
{
     FILE *ffile;
     char xf[132];
     
     init();			/* Initialize tables */
     
     args(argc, argv);		/* Decode arguments */
     
     ffile = stdout;
     
     if(opt_rewrite)
      {
	   sprintf(xf, ".fmt%X", getpid());
	   
	   if(link(SAD_FORMATS, xf) < 0 ||
	      unlink(SAD_FORMATS) < 0 ||
	      !(ffile = fopen(SAD_FORMATS, "w")))
	    {
		 perror(SAD_FORMATS);
		 exit(1);
	    }
      }
     
     if(opt_join)
	  join_formats((opt_rewrite ? xf : SAD_FORMATS), ffile);
     else
	  process_formats((opt_rewrite ? xf : SAD_FORMATS),
			  opt_addr, opt_fmt, ffile);
     
     if(opt_rewrite)
      {
	   fclose(ffile);
	   
	   if(!opt_debug)
		unlink(xf);
      }
}
