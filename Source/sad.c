/* sad.c -- SAD main programs.

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
#include "macros.h"
#include "formats.h"
#include "globals.h"

/* Set up for new instruction */
void new_instr()
{
     l_code[0] = l_instr[0] = l_comment[0] = 0;
     org_pc = pc;
     l_codep = l_code;
     l_commentp = l_comment;
     mode_single_op = FALSE;
}


/* Temporarily set format to code */
void format_code()
{
     static struct format f;
  
     new_mode_rpl=FALSE;
     branch=FALSE;		/* Assumes new stream is starting */
     if(curfmtref && *curfmtref)
      {
	   f.nformats = 1;
	   f.fmtspec[0].ftype = FT_CODE;
	f.fmtspec[0].width = 1;
	   f.fmtspec[0].repeat = 1;
	   curfmt = &f;
      }
}



/* Open core file */
static void open_core(fname)
    char *fname;
{
     if(!(core_file = fopen(fname, "r")))
      {
	   perror(fname);
	   exit(1);
      }
     
     fseek(core_file,0L,2);
     coresize=ftell(core_file);
     
     if(fseek(core_file, (long) (opt_startpt-baseaddr), 0) < 0)
      {
	   error("address %X not in corefile.\n", opt_startpt);
	   exit(1);
      }
     if(baseaddr==0)
	  if (fetch_unibbles(0x00029,5) == 0)
	   {
		opt_gx = TRUE;
		ram_base = 0x80000;
	   }
	  else
	   {
		opt_gx = FALSE;
		ram_base = 0x70000;
	   }
     if( opt_hp49g )
       ram_base = 0x80000; /* just a HP49G kludge */
}

/* ********************************************************************** */
/* *               Misc utilities for argument checking                 * */
/* ********************************************************************** */

/* Seek entry name, assume it exists */
static int entry_to_loc(name)
    char *name;
{
     int loc, addr;
     
     if(!symref || !symroot || !nsymbols)
      {
	   error("No symbol table to look for %s.\n", name);
	   exit(1);
      }
     
     for(loc=0 ; loc<nsymbols ; loc++)
      {
	   if( symref[loc]->id != NULL )
		if( strcmp(symref[loc]->id, name) == 0)
		     break;
		else
		     ;
      }
	   
     if(loc==nsymbols)
      {
	   error("No entry %s in symbol table.\n", name);
	   exit(1);
      }

     /* Got start address */
     addr = symref[loc]->val;

     /* Disallow romptr names */
     if(addr > 0xFFFFF)
      {
	   error("Symbol not entry: %s\n", name);
	   exit(1);
      }
     
     return(loc);
}

/* Guess end entry based on object at addr, symbol table loc */
static int guess_end(addr, loc)
    int addr, loc;
{
     int endaddr;

     if(prologp(fetch_unibbles(addr,5)))
      {
	   /* Skip rpl object to get end address */
	   set_pc(addr);
	   endaddr = skipob(FALSE);
      }
     else
      {
	   /* Not rpl object, end at next symbol */
	   /* Do not allow very close entries */
	   /* Chose 5 as a suitable limit */
	   for(;
	       (++loc<nsymbols) &&
	       ( (endaddr=symref[loc]->val)-addr <= 5)
	       ;);
	   /* Possible improvement is to stop at next prolog */
	   /* or PCO */
      }
     return(endaddr);
}



/* ********************************************************************** */
/* *               Main disassembler loop                               * */
/* ********************************************************************** */

/* Disassemble according to current format */
static void decode()
{
     new_mode_rpl=TRUE;

     /* If code */
     if(!curfmt || !curfmt->nformats ||	(curfmt->fmtspec[0].ftype & FT_CODE))
      {
	   new_mode_rpl=FALSE;
	   if(!gosub)
		 decode_instr();
	   else
	    {
		 gosub=FALSE;
		 if (!opt_gx && !opt_hp49g)
		      switch(jumpaddr)
		       {
		       case 0xC8DE:	/* GOTO:    */
		       case 0xC74D:	/* CKGOSUB: */
		       case 0xC783:	/* GOSUB:   */
		       case 0xC975:	/* MLGOSUB: */
			    OUTCON5(SYMB_JMP(get_unibbles(5)));
			    jumpaddr=0;
			    break;
		       default:
			    decode_instr();
			    break;
		       }
		 else
		      decode_instr();
	    }
	   if(opt_printml)
		print_line();
	   return;
      }

     switch(curfmt->fmtspec[0].ftype)
      {
      case FT_RPL:
	   decode_rpl();
	   break;
      case FT_VAR:
	   decode_variable();
	   break;
      case FT_HASH:
	   decode_hashtable();
	   break;
      case FT_LINK:
	   decode_linktable();
	   break;
      case FT_LIB:
	   decode_romlibrary();
	   break;
      case FT_CRC:
	   decode_crc();
	   break;
      default:
	   decode_data(curfmt);
	   break;
      }
}

/* Run one disassembly pass */
static void disassemble(npass)
  int npass;
{
     struct format
	  code_fmt, rpl_fmt;
     
     code_fmt.nformats = 1;
     code_fmt.fmtspec[0].ftype = FT_CODE;
     code_fmt.fmtspec[0].fmtdescr = NULL;
     code_fmt.fmtspec[0].width = 1;
     code_fmt.fmtspec[0].repeat = 1;
     rpl_fmt.nformats = 1;
     rpl_fmt.fmtspec[0].ftype = FT_RPL;
     rpl_fmt.fmtspec[0].fmtdescr = NULL;
     rpl_fmt.fmtspec[0].width = 1;
     rpl_fmt.fmtspec[0].repeat = 1;
     
     pass = npass;
     lineno = 0;
     nextsym = symref;
     nextcom = comref;
     nextfmt = fmtref;
     curfmt = NULL;
     
     set_pc(opt_startpt);
     
     rpl_comp_level = rpl_ind_level = rpl_new_ind_level = 0;
     mode_rpl = new_mode_rpl = TRUE;
     mode_endcode = FALSE;
     mode_single_op = mode_fixed_line = mode_dispatch = FALSE;
     dispatch_obs = dispatch_level = dispatch_lines = 0;
     branch = FALSE;

     last_cpc = last_apc = 0;

     l_sourcep = l_source;
     source_ind = 0;

     /* Find or guess starting format */
     for(; nextsym && *nextsym && ((*nextsym)->val < pc); nextsym++);
     for(; nextcom && *nextcom && ((*nextcom)->val < pc); nextcom++);
     for(; nextfmt && *nextfmt && ((*nextfmt)->val <= pc); nextfmt++)
      {
	   curfmt = (*nextfmt)->form;
	   curfmtref = nextfmt;
      }

     if ( (curfmtref && *curfmtref && (*curfmtref)->val != pc) || !curfmt )
      {
	   /* Make a guess */
	   int tag = fetch_unibbles(pc,5);
	   int tag2 = fetch_unibbles(pc+5,5);

	   if( prologp( tag ) ||
               ( tag == pc + 5 ) ||
               ( is_header() != 0 ) ||
	       ( ( !opt_hp49g ) &&
                 ( tag == 0x641CC ) &&
                 ( prologp( tag2 ) || ( tag2 = pc + 10 ) )))

		curfmt = &rpl_fmt;
	   else
		curfmt = &code_fmt;
      }		 
     
     if(pass == PASS2 & opt_code & !opt_opcode)
      {
	   separate(FALSE);
	   printf("ASSEMBLE\n\tABS\t%X\nRPL\n", opt_startpt);
	   separate(FALSE);
      }

     while(pc < opt_endpt)
      {

	   /* Is the current format temporary, and if so, has it expired? */
	   /* Has to be here in order not to override .formats declarations */
	   if(prevfmt && (pc >= fmtexpire_tag))
	    {
		 curfmt= prevfmt;
		 prevfmt = NULL;
		 fmtexpire_tag = 0xffffff;
		 rpl_ind_level--;
		 rpl_new_ind_level--;
		 if(curfmt->fmtspec[0].ftype & FT_RPL)
		  {
		       new_instr();
		       new_mode_rpl=mode_rpl=TRUE;
		       mode_single_op = TRUE ;
		       mode_endcode = TRUE;
		       OUT1("ENDCODE");
		       print_line();
		  }
	    }
	   mode_endcode=FALSE;


	   /* Skip to next symbol, comment, and format to look for */
	   for(; nextsym && *nextsym && ((*nextsym)->val < pc); nextsym++);
	   for(; nextcom && *nextcom && ((*nextcom)->val < pc); nextcom++);
	   for(; nextfmt && *nextfmt && ((*nextfmt)->val <= pc); nextfmt++)
	    {
		 curfmt = (*nextfmt)->form;
		 curfmtref = nextfmt;
	    }

	   if(!curfmt)
		switch((*curfmtref)->type & T_FMT_TYPE)
		 {
		 case T_FMT_CODE:
		      curfmt = &code_fmt;
		      break;
		 case T_FMT_RPL: 
		      curfmt = &rpl_fmt;
		      break;
		 case T_FMT_VAR:
		      curfmt = &rpl_fmt;
		      rpl_fmt.fmtspec[0].ftype=FT_VAR;
		      break;
		 case T_FMT_HASH:
		      curfmt = &rpl_fmt;
		      rpl_fmt.fmtspec[0].ftype=FT_HASH;
		      break;
		 case T_FMT_LINK:
		      rpl_fmt.fmtspec[0].ftype=FT_LINK;
		      curfmt = &rpl_fmt;
		      break;
		 case T_FMT_LIB:
		      rpl_fmt.fmtspec[0].ftype=FT_LIB;
		      curfmt = &rpl_fmt;
		      break;
		 case T_FMT_CRC:
		      rpl_fmt.fmtspec[0].ftype=FT_CRC;
		      curfmt = &rpl_fmt;
		      break;
		 }
	   else	if(( (*curfmtref)->val != pc) &&
		   (!curfmt->nformats || (curfmt->fmtspec[0].ftype & FT_CODE)))
	    {
		 /* See if rpl is starting */
		 int tag = get_pcunibbles(5);
		 int tag2 = get_pcunibbles(5);
		 set_pc(pc-10);
		 if( ( force_prologp( tag ) || ( tag == pc + 5 ) ) ||
		     ( ( !opt_hp49g ) &&
                       ( tag == 0x641CC ) &&
                       ( prologp( tag2 ) || ( tag2 = pc + 10 ) )))
		  {
		       curfmt = &rpl_fmt;
		       /* Quit entry mode if active */
		       if(opt_entry)
			{
			     opt_entry = FALSE;
			     opt_endpt = pc;
			}
		  }
	    }

	   new_instr();		/* Set up for new instruction */
	   decode();		/* Decode */

	   /* Resuming normal rpl from other rpl modes */
	   rpl_fmt.fmtspec[0].ftype = FT_RPL;
	   if(curfmt)
		switch(curfmt->fmtspec[0].ftype)
		 {
		 case FT_LIB:
		 case FT_HASH:
		 case FT_LINK:
		 case FT_VAR:
		 case FT_CRC:
		      curfmt = &rpl_fmt;
		      break;
		 default:
		      break;
		 }
      }
     print_source();		/* Print remaining source */
}


/* ********************************************************************** */
/* *               Print usage                                          * */
/* ********************************************************************** */

/* Print usage */

void usage(argc, argv)
     int argc;
     char **argv;
{
     fprintf(stderr, "This is SAD%s (%s)\n", sad_version, *argv);
     fprintf(stderr, "usage: sad [-acdefghjsxzACGHLX18]{entry {len} | start end }\n");
     fprintf(stderr, "       a  Assembler format\n");
     fprintf(stderr, "       A  Rplcomp format\n");
     fprintf(stderr, "       c  Suppression of disassembler comments\n");
     fprintf(stderr, "       d  Insert definitions for unsupported symbols\n");
     fprintf(stderr, "          dd = for all used symbols\n");
     fprintf(stderr, "       e  Disassemble named entry\n");
     fprintf(stderr, "       f  Repeated F pass. Output to formats.out\n");
     fprintf(stderr, "       L  Don't generate local symbols.\n");
     fprintf(stderr, "       h  Supported entries only\n");
     fprintf(stderr, "       H  Do not use suspected references\n");
     fprintf(stderr, "       s  Symbolic addresses are moved to comments\n");
     fprintf(stderr, "       x  Add cross reference to the end.\n");
     fprintf(stderr, "       z  Alonzo mode\n");
     fprintf(stderr, "       1  One pass only. Skips local symbols.\n");
     fprintf(stderr, "       C  No code output.\n");
     fprintf(stderr, "       j  Jumpify.\n");
     fprintf(stderr, "       g  Grobs drawn after source lines.\n");
     fprintf(stderr, "       G  Grobs not drawn\n");
     fprintf(stderr, "       M  No machine language output.\n");
     fprintf(stderr, "       8  Binary is for HP48 (ie. any 48).\n");
     fprintf(stderr, "       X  Binary is for GX.\n");
/*   fprintf(stderr, "       i  Intelligent mode\n"); */
     exit(1);
}


/* ********************************************************************** */
/* *               Read arguments                                       * */
/* ********************************************************************** */

/* Parse arguments */
static void args(argc, argv)
    int argc;
    char **argv;
{
     char *cp1, *cp2;
     int flagsp = 0;
     
     if(argc > 1 && argv[1][0] == '-')
      {
	   flagsp = 1;
	   
	   for(cp1 = argv[1]+1; *cp1; cp1++)
		switch(*cp1)
		 {
		 case 'X':		/* Binary is for GX */
		      opt_gx = !opt_gx;
		      break;
		 case '1':		/* Skip pass one */
		      opt_pass1 = !opt_pass1;
		      break;
		 case '8':		/* Binary is for 48 */
		      opt_gx = !opt_hp49g;
		      break;
		 case 'Q':		/* Debug */
		      opt_debug = !opt_debug;
		      break;
		 case 'f':		/* Auto format generation */
		      opt_formats = !opt_formats;
		      break;
		 case 'C':		/* Code */
		      opt_code = !opt_code;
		      break;
		 case 'M':              /* Print machine language */
		      opt_printml = !opt_printml;
		      break;
		 case 'L':		/* Symbol generation */
		      opt_locals = !opt_locals;
		      break;
		 case 'a':		/* Assembler format - opcodes (off) */
		      opt_opcode = !opt_opcode;
		      break;
		 case 's':		/* (No) symbols in listings */
		      opt_symbols = !opt_symbols;
		      break;
		 case 'd':		/* (No) undef-but-ref  symbol defs */
		      if(!opt_symdef)
			   opt_symdef = !opt_symdef;
		      else
			   opt_symdef_all = !opt_symdef_all;
		      break;
		 case 'x':		/* Full XREF */
		      opt_xref = !opt_xref;
		      break;
		 case 'c':		/* Auto comments */
		      opt_comments = !opt_comments;
		      break;
		 case 'e':		/* Start entry is argument */
		      opt_entry = !opt_entry;
		      break;
		 case 'z':		/* Alonzo-styled formatting */
		      opt_alonzo = !opt_alonzo;
		      break;
		 case 'h':		/* Supported entries only */
		      opt_hp = !opt_hp;
		      break;
		 case 'H':		/* Suspected references */
		      opt_comref = !opt_comref;
		      break;
		 case 'j':		/* Jumpify */
		      opt_jumpify = !opt_jumpify;
		      break;
		 case 'g':		/* Grob drawing after source? */
		      opt_groblast = !opt_groblast;
		      break;
		 case 'G':              /* Grob drawing */
		      opt_drawgrob = !opt_drawgrob;
		      break;
		 case 'A':		/* Source code-like disassembly */
		      opt_source = !opt_source;
		      break;
/* Not implemeted yet
 *		 case 'i':
 *
 *		      opt_intelligent = !opt_intelligent;
 *		      break;
 */
		 default:
		      usage(argc, argv);
		 }
      }

     if(!opt_entry)
      {
	   if(argc != 3+flagsp)
		usage(argc, argv);
	   cp1 = argv[++flagsp]; 
	   cp2 = argv[++flagsp];
	   if(!hexstrtoi(&cp1, &opt_startpt) ||
		!hexstrtoi(&cp2, &opt_endpt) ||
		*cp1 != '\0' || *cp2 != '\0')
		usage(argc, argv);
      }
     else
      {
	   arg_entry = ++flagsp;
	   if(argc == 2+flagsp)
	    {
		 /* Parse hxs, if error then assume end entry */
		 cp2 = argv[++flagsp];
		 if(!hexstrtoi(&cp2, &opt_endpt))
		      opt_entryend = TRUE;
		 else
		      opt_entrylen = TRUE;
	    }
	   else if(argc != 1+flagsp)
		usage(argc, argv);
      }

     if(opt_alonzo)
      {
	   opt_commentcol += 8;
	   opt_opcode = 1;
      }

     if(opt_source)
      {
	   /* Set source code mode */
	   opt_pass1 = TRUE ;
	   opt_printml = TRUE ;
	   opt_locals = TRUE ;
	   opt_opcode = FALSE ;
	   opt_symbols = TRUE ;
	   opt_comments = FALSE;
      }
     if(opt_debug)
	  fprintf(stderr, "Debug mode enabled.\nThis is SAD%s (%s)\n\n",
		  sad_version, *argv);
}  


/* ********************************************************************** */
/* *               Main program                                         * */
/* ********************************************************************** */

int main( int argc, char **argv )
{

   signal( SIGSEGV, sighandle );
   signal( SIGBUS, sighandle );

   init();			/* Initialize tables */

   args( argc, argv );		/* Decode arguments */
     
   if( opt_hp49g ) {
      open_core( SAD_CORE49G );
      load_comments( SAD_COMMENTS49G );
      load_formats( SAD_FORMATS49G );
      load_symbols( symcmp );
   } else if( opt_startpt <= 0x7ffff ) {
      if(opt_gx) {
         open_core( SAD_COREGX );
	 load_comments( SAD_COMMENTSGX );
	 load_formats( SAD_FORMATSGX );
      } else {
	 open_core( SAD_CORE );
	 load_comments( SAD_COMMENTS );
	 load_formats( SAD_FORMATS );
      }
      load_symbols( symcmp );
   } else if( opt_startpt <= 0xbffff ) {
      baseaddr = 0x80000;
      open_core( SAD_CORE1 );
      load_symbols( symcmp );
      load_comments( SAD_COMMENTS1 );
      load_formats( SAD_FORMATS1 );
   } else {
      baseaddr = 0xc0000;
      open_core( SAD_CORE2 );
      load_symbols( symcmp );
      load_comments( SAD_COMMENTS2 );
      load_formats( SAD_FORMATS2 );
   }
   load_macros( SAD_MACROS );

   /* Set RAM base address for guessing RAM variable addresses */
   if( opt_gx || opt_hp49g )
      ram_base = 0x80000;
   else
      ram_base = 0x70000;

     /* If entry is cmdline argument find symbols address */
     if(opt_entry)
      {
	   int loc;

	   /* Not needed anymore */
	   opt_entry = FALSE;

	   loc = entry_to_loc(argv[arg_entry]);

	   opt_startpt = symref[loc]->val;
	   
	   if(opt_entrylen)
		opt_endpt += opt_startpt;
	   else if (opt_entryend)
	    {
		 loc = entry_to_loc(argv[arg_entry+1]);
		 opt_endpt = symref[loc]->val;
		 
		 /* Include disassembly of entryend too */
		 opt_endpt = guess_end(opt_endpt, loc);
		 
	    }
	   else
	    {
		 /* Only start entry given, guess end address */
		 
		 opt_endpt = guess_end(opt_startpt, loc);
	    }
      }

     if(opt_endpt > coresize+baseaddr)
	  opt_endpt=coresize+baseaddr;
     
     if(opt_endpt < opt_startpt)
      {
	   error("Disassembly end address (#%X) < start address (#%X).\n",
		 opt_endpt, opt_startpt);
	   exit(1);
      }

     if(opt_pass1)
	  disassemble(PASS1);		/* Run pass 1 */

     add_locals_to_symtab(symcmp);	/* Add local symbols to table */
     
     if(opt_symdef && opt_pass1)
	  print_sym_defs();
     /* Repeat pass F until no changes */
     if(opt_formats)
	  do
	   {
		nautoformats = 0;
		format_added = FALSE;
		
		disassemble(PASSF);
		
		if(nautoformats)
		     add_autos_to_fmttab();
	   }
     while(format_added);
     
     disassemble(PASS2);	      /* Print output and collect XREF info */
     
     if(opt_symdef && !opt_pass1)
	  print_sym_defs();
     
     /* Define referenced undefined symbols */
     
     if(opt_xref)
	  print_xref();		      /* Print full xref info */
     
     if(opt_formats)
	  print_format_file(SAD_AUTOFMT); /* Print auto formats */
}
