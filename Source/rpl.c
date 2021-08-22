/* rpl.c -- SAD rpl disassembler programs.

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

/* Lots of jumping around, need to declare this */

static void disass_body();

/* ********************************************************************** */
/* *               Miscellaneous RPL utils                              * */
/* ********************************************************************** */

/* Return TRUE if prolog */
int prologp(prolog)
    int prolog;
{
     static int prlg[] = {
	   SYSTEM_BINARY,
	   REAL_NUMBER,
	   LONG_REAL,
	   COMPLEX,
	   LONG_COMPLEX,
	   CHARACTER,
	   STRING,
	   BINARY_INT,
	   GLOBAL_NAME,
	   LOCAL_NAME,
	   ROMPTR,
	   LIST,
	   ALGEBRAIC,
	   UNIT,
	   TAGGED,
	   PROGRAM,
	   CODE,
	   DIRECTORY,
	   ARRAY,
	   GROB,
	   LINKED_ARRAY,
	   BACKUP,
	   LIBRARY_DATA,
	   ACPTR,
	   EXT1,
	   EXT2,
	   EXT3,
	   LIBRARY,
	   FLASHPTR,
	   LONG_INT,
	   0
	   };
     int *p = prlg;

     do
	  if(prolog == *p)
	       return(TRUE);
     while(*++p);

     return(FALSE);
}


/* Return TRUE if prolog forces RPL mode */
int force_prologp(prolog)
    int prolog;
{
     /* Prologs that frequently end ml mode */

     static int prlg[] = {
	   BINARY_INT,
	   LIST,
	   ALGEBRAIC,
	   PROGRAM,
	   CODE,
	   GROB,		/* Unlikely */
	   0
	   };
     int *p = prlg;

     do
	  if(prolog == *p)
	       return(TRUE);
     while(*++p);

     return(FALSE);
}


/* Skip object and return new PC, FALSE if cannot skip */
int skipob(prolog)
    int prolog;
{
     int old_pc, new_pc;

     old_pc=pc;

     if(!prolog)
	  prolog = get_pcunibbles(5);

     new_pc = pc;
     switch(prolog)
      {
      case SYSTEM_BINARY:	new_pc += 5 ; break;
      case REAL_NUMBER:		new_pc += 16; break;
      case LONG_REAL:		new_pc += 21; break;
      case COMPLEX:		new_pc += 32; break;
      case LONG_COMPLEX:	new_pc += 64; break;
      case CHARACTER:		new_pc += 2 ; break;
      case ROMPTR:		new_pc += 6 ; break;
      case FLASHPTR:		new_pc += 7 ; break;

      case GLOBAL_NAME:
      case LOCAL_NAME:
	   new_pc += get_pcunibbles(2)*2+2;
	   break;
      case STRING:
      case BINARY_INT:
      case LONG_INT:
      case CODE:
      case ARRAY:
      case GROB:
      case LINKED_ARRAY:
      case LIBRARY_DATA:
      case EXT2:
      case EXT3:
      case BACKUP:
      case LIBRARY:
	   new_pc += get_pcunibbles(5);
	   break;
      case TAGGED:
	   new_pc += get_pcunibbles(2)*2+2;
	   set_pc(new_pc);
	   new_pc = skipob(FALSE);
	   break;
      case LIST:
      case ALGEBRAIC:
      case UNIT:
      case PROGRAM:
	   while((prolog=get_pcunibbles(5)) != SEMI)
		set_pc(new_pc=skipob(prolog));
	   new_pc += 5;
	   break;
      case DIRECTORY:
      {
	   int len;
	   new_pc += 7;
	   len = fetch_unibbles(new_pc,5);
	   if(len==0)
		new_pc += 5;
	   else
	    {
		 new_pc += len;
		 len = fetch_unibbles(new_pc,2);
		 if(len==0)
		      new_pc += 2;
		 else
		      new_pc += len*2+4;
		 set_pc(new_pc);
		 new_pc = skipob(FALSE);
	    }
	   break;
      }
      case EXT1:
	   if(opt_gx || opt_hp49g)
		new_pc += 10;
	   else
		new_pc += get_pcunibbles(5);
	   break;
      default:
	   break;
      }
     
     set_pc(old_pc);
     return (new_pc);
}

/* ********************************************************************** */
/* *               Variable field disassembling                         * */
/* ********************************************************************** */

/* Decode variable */
void decode_variable()
{
     int offset, id_len;
     
     separate(FALSE);
     offset = get_unibbles(5);
     new_mode_rpl=FALSE;
     if(offset)
	  OUT2("CON(5)\t(*)-%s", SYMB_ABS(pc-offset-5));
     else
	  OUTCON0();
     comment("Offset to prev. var.");
     print_line();
     
     new_instr();
     id_len = get_unibbles(2);
     OUT2("CON(2)\t%d", id_len);
     print_line();
     
     if(id_len>0)
      {
	   new_instr();
	   OUT2("NIBASC\t'%s'",make_ascstr(id_len,HPCHR,YESSPC));
	   print_line();
	   
	   new_instr();
	   OUT2("CON(2)\t%d", get_unibbles(2));
	   print_line();
      }
}

/* ********************************************************************** */
/* *               Link table                                           * */
/* ********************************************************************** */

void decode_linktable()
{
     int prolog, tablen, tabend, offset, rompnum;

     if(opt_source & (libnum!=0))
      {
	   prolog = get_unibbles(5);
	   if(prolog == SYSTEM_BINARY)
		set_pc(pc+5);
	   else if(prolog == BINARY_INT)
	    {
		 tablen = get_unibbles(5);
		 set_pc(pc+tablen-5);
	    }
	   return;
      }

     new_instr();
     prolog = get_unibbles(5);
     if(prolog == SYSTEM_BINARY)
      {
	   int body = get_unibbles(5);
	   OUT2("# %X", body);
	   comment("Table at ",SYMB_RPL(body));
	   print_line();
	   return;
      }
     if(prolog != BINARY_INT)
      {
	   disass_body(prolog, TRUE);
	   return;
      }

     new_mode_rpl=FALSE;
     OUTCON5(SYMB_ABS(prolog));
     print_line();

     new_instr();
     tablen=get_unibbles(5);
     tabend=pc+tablen-5;
     OUTREL5(SYMB_ABS(tabend));
     print_line();
     
     rompnum=0;
     while((pc < tabend) && (pc < opt_endpt))
      {
	   new_instr();
	   offset=get_nibbles(5);
	   if(offset)
	    {
		 OUTREL5(SYMB_ABS(pc-5+offset));
		 comment("Offset to romptr %d", rompnum);
	    }
	   else
	    {
		 OUTCON0();
		 comment("No romptr %d", rompnum);
	    }
	   print_line();
	   rompnum++;
      }
}


/* ********************************************************************** */
/* *               Hash table                                           * */
/* ********************************************************************** */

void decode_hashtable()
{
     int prolog, tablen, tabend, nameslen, namesend, namelen, offset, rompnum;
     
     new_instr();
     if(opt_source & (libnum!=0))
      {
	   prolog = get_unibbles(5);
	   if(prolog == SYSTEM_BINARY)
		set_pc(pc+5);
	   else if(opt_gx & (prolog == EXT1))
		set_pc(pc+10);
	   else if(prolog == BINARY_INT)
	    {
		 tablen = get_unibbles(5);
		 set_pc(pc+tablen-5);
	    }
	   return;
      }

     new_instr();
     prolog = get_unibbles(5);
     if(prolog == SYSTEM_BINARY)
      {
	   int body = get_unibbles(5);
	   OUT2("# %X", body);
	   comment("Table at ",SYMB_RPL(body));
	   print_line();
	   return;
      }
     if(prolog != BINARY_INT)
      {
	   disass_body(prolog, TRUE);
	   return;
      }

     new_mode_rpl=FALSE;
     OUTCON5(SYMB_ABS(prolog));
     print_line();

     new_instr();
     tablen=get_unibbles(5);
     tabend=pc+tablen-5;
     OUTREL5(SYMB_ABS(tabend));
     print_line();

     for(namelen=1 ; namelen <= 16 ; namelen++)
      {
	   new_instr();
	   offset=get_unibbles(5);
	   if(offset)
	    {
		 OUTREL5(SYMB_ABS(pc+offset-5));
		 comment("Offset to names of len %d", namelen);
	    }
	   else
	    {
		 OUTCON0();
		 comment("No names of len %d",namelen);
	    }
	   print_line();
      }

     new_instr();
     nameslen=get_unibbles(5);
     namesend=pc+nameslen-5;
     OUTREL5(SYMB_ABS(pc+nameslen-5));
     comment("Lenght of name table");
     print_line();

     while((pc<namesend) && (pc < tabend) && (pc < opt_endpt))
      {
	   new_instr();
	   namelen=get_unibbles(2);
	   OUT2("CON(2)\t%d", namelen);
	   print_line();

	   if(namelen)
	    {
		 new_instr();
		 OUT2("NIBASC\t'%s'", make_ascstr(namelen,HPCHR,YESSPC));
		 print_line();
	    }

	   new_instr();
	   rompnum=get_unibbles(3);
	   OUT2("CON(3)\t#%X", rompnum);
	   print_line();
      }

     rompnum=0;
     while((pc < tabend) && (pc < opt_endpt))
      {
	   new_instr();
	   offset=get_unibbles(5);
	   if(offset)
	    {
		 OUT2("CON(5)\t(*)-(%s)", SYMB_ABS(pc-5-offset));
		 comment("Offset to name %d", rompnum);
	    }
	   else
	    {
		 OUTCON0();
		 comment("Romptr %d unnamed", rompnum);
	    }
	   print_line();
	   rompnum++;
      }
}

/* ********************************************************************** */
/* *               Miscellaneous                                        * */
/* ********************************************************************** */

/* Objects with hex data body */
static void disass_bodyhex(prolog, wid, rpl, rel)
    int prolog, wid, rpl, rel;
{
     int len;

     new_mode_rpl=FALSE;
     if(rpl)
      {
	   OUTCON5(SYMB_ABS(prolog));
	   print_line();
	   new_instr();
      }
     len = get_unibbles(wid);

     if(rel)
	  OUT3("REL(%d)\t%s", wid, SYMB_ABS(pc+len-wid));
     else
	  OUT3("CON(%d)\t#%X", wid, len);
     print_line();

     if(rel && (len-wid>0))
      {
	   len -= wid;
	   while(len>0)
	    {
		 new_instr();
		 if(len>=40)
		      OUTNIBHEX(40);
		 else
		      OUTNIBHEX(len);
		 len-=40;
		 print_line();
	    }
      }
     else if (len>0)
      {
	   new_instr();
	   OUTNIBHEX(len);
	   print_line();
      }
}



/* Objects with asc data body */
static void disass_bodyasc(prolog, wid, rpl, rel)
    int prolog, wid, rpl, rel;
{
     int len = get_unibbles(wid);

     new_mode_rpl=FALSE;
     if(rpl)
      {
	   OUTCON5(SYMB_ABS(prolog));
	   print_line();
	   new_instr();
      }
     if(rel)
	  OUT3("REL(%d)\t%s", wid, SYMB_ABS(pc+len-wid));
     else
	  OUT3("CON(%d)\t#%X", wid, len);
     print_line();
     
     if(rel)
      {	
	   if((len-wid)/2>0)
	    {
		 new_instr();
		 OUT2("NIBASC\t'%s'", make_ascstr((len-wid)/2,HPCHR,YESSPC));
		 print_line();
	    }
      }
     else if(len)
      {
	   new_instr();
	   OUT2("NIBASC\t'%s'", make_ascstr(len, HPCHR,YESSPC));
	   print_line();
      }
     if(rel & !(len & 1))
      {
	   /* Illegal string */
	   new_instr();
	   OUT2("CON(1)\t%s", CKSYMB_ABS(get_unibbles(1)));
	   print_line();
      }
}


int is_header()
{
     int libnumpart, cmdnumpart;
     int propsize=0;

     /* Disallow xNAMEs inside composites */
     /* Some programs purposefully change the last SEMI in a dispatch structure
      * to something else that causes a Bad Argument Error.
      * Haven't figured a safe way to get around this yet
      * (Link table isn't always available for the guess..)
      */
     if((rpl_comp_level!=0) & !mode_imaginary_composite)
	  return(0);
     if(pc+14 >= opt_endpt)
	  return(0);
     
     if(fetch_unibbles(pc+7,5)==PROGRAM)
	  propsize=1;
     else if(fetch_unibbles(pc+9,5)==PROGRAM)
	  propsize=3;
     else if(opt_gx & (fetch_unibbles(pc+7,5)==ROMPTR))
	  if(fetch_unibbles(pc+12,3)==0xF1)
	       propsize=1;
     
     if(propsize==0)
	  return(0);

     libnumpart=fetch_unibbles(pc+propsize,3);
     cmdnumpart=fetch_unibbles(pc+propsize+3,3);

     if((libnumpart==0) & (cmdnumpart==0))
	  return(0);
     if(libnumpart>0x7FF)
	  return(0);
     if(cmdnumpart>400)
	  return(0);
     return(propsize);
}

int get_relative( int num, int width )
{
   unsigned int mask = 0xF, i;

   for( i = 1; i < width; i++) {
      mask <<= 4;
      mask += 0xF;
   }
   return ( num & mask );
}

static void decode_header(propsize)
    int propsize;
{
     int libnumpart, cmdnumpart, proppart, prop;

     new_mode_rpl=FALSE;
     separate(FALSE);
     new_instr();
     proppart=get_unibbles(propsize);
     OUT3("CON(%d)\t#%X", propsize, proppart);
     comment("Romptr property flags");
     print_line();
     new_instr();
     libnumpart=get_unibbles(3);
     if(!opt_source | (libnum==0) )
      {
	   OUT2("CON(3)\t#%03X", libnumpart);
	   comment("Library number");
	   print_line();
      }
     new_instr();
     cmdnumpart=get_unibbles(3);
     if(!opt_source | (libnum==0))
      {
	   OUT2("CON(3)\t#%03X", cmdnumpart);
	   comment("Command number");
	   print_line();
      }
     if((pass == PASS2) && opt_code)
	  if(libnumpart >= 0x700)
	   {
		printf("* Macro properties:");
		printf(" %s", macro_prop[proppart & 3]);
		if(proppart & 0x8)
		     printf(", %s", macro_prop[4]);
		if(proppart & 0x4)
		     printf(", %s", macro_prop[5]);
		putchar('\n');
		
	   }
	  else
	   {
		if((propsize==1) & (proppart == 0x8))
		     return;
		if(propsize==1)
		     proppart *= 0x100;
		if(propsize==3)
		     printf("* Function properties:");
		else
		     printf("* Command properties:");
		if(proppart == 0)
		     printf(" none");
		else
		     for(prop=0 ; (prop<11) && (proppart!=0) ; prop++)
		      {
			   proppart = (proppart*2)&0xFFF;
			   if(proppart & 0x800)
				printf(" %s,", function_prop[prop]);
		      }
		putchar('\n');
	   }
     return;
}


/* ********************************************************************** */
/* *               Library disassembling                                * */
/* ********************************************************************** */


/* Return ROMPTR name from hash table */
static char *hash_name( int hashloc, int num )
{

   static char hashname[50];
   char *cp ;
   int old_pc, chr;
   int hashend, hashoff, len, nameslen, hashprlg;
     
   if( hashloc == 0 ) {
      sprintf( hashname, "XLIB_%X_%X", libnum, num );
      return( hashname );
   }
   cp = hashname;
   old_pc = pc;
   set_pc( hashloc );

   hashprlg = get_pcunibbles( 5 );
   /* System binary points to a table in hidden ROM */
   if( hashprlg == SYSTEM_BINARY )
      set_pc( get_pcunibbles( 5 ) );

   /* ACPTR points to a table */
   if( ( opt_gx || opt_hp49g ) && ( hashprlg == EXT1 ) ) {
      hashloc = get_pcunibbles( 5 );
      if( hashloc > coresize ) {
	 sprintf( hashname, "XLIB_%X_%X", libnum, num );
	 return( hashname );
      }
      set_pc( hashloc + 5 );
   }


   /* Now we assume we have a hex str like ROMPTR>ID */

   hashend = hashloc + 5 + get_pcunibbles( 5 );
   /* Internally 0x55 is added but now pc has advanced 5 */
   set_pc( pc + 0x50 );
   /* Skipping to offset location */
   nameslen = get_pcunibbles( 5 );
   if( hashend <= ( pc + nameslen + 5 * num - 5 ) ) {
      sprintf( hashname, "XLIB_%X_%X", libnum, num );
      set_pc( old_pc );
      return( hashname );
   }
   set_pc( pc + nameslen + 5 * num - 5 );
   hashoff = get_pcunibbles( 5 );
   if( hashoff == 0 ) {
      sprintf( hashname, "xXLIB_%X_%X", libnum, num );
      set_pc( old_pc );
      return( hashname );
   }
   set_pc( pc - 5 - hashoff );
   /* Using the common naming convention for xNAMEs */
   *cp++ = 'x';
   strcpy( cp, make_ascstr( get_pcunibbles( 2 ), HPCHR, NOSPC ) );
   set_pc( old_pc );
   return( hashname );
}


/* Disassemble library headers and tables */
static void disass_library( int rpl, int crcp )
{
   int libsize, libidsize;
   int hashoff, mesgoff, linkoff, cfgoff;
   int mesgloc, cfgloc;
   int libtemp;

   static char tempname[20];

   if( !opt_source )
      new_mode_rpl = FALSE;
   if( rpl && !opt_source ) {
      OUTCON5( SYMB_ABS( LIBRARY ) );
      print_line();
      new_instr();
   }     
   
   /* Print & mark library end */
   libsize = get_unibbles( 5 );
   libend = pc + libsize - 5;

   /* Fetching libnum */
   libidsize = fetch_unibbles( pc, 2 );
   if( libidsize == 0 )
      libnum = fetch_unibbles( pc + 2, 3 );
   else
      libnum = fetch_unibbles( pc + 2 * libidsize + 4 , 3 );

   if( ( pass == PASS1 ) &&
       ( opt_startpt <= libend ) && ( libend <= opt_endpt ) ) {
      if( libend <= 0xFFFFF ) {
         add_auto_format( libend, T_FMT_RPL );
         if( crcp )
	    add_auto_format( libend - 4, T_FMT_CRC );
      } else {
	 /* this part of ifelse is a HUGE KLUDGE */
         if( libidsize == 0 )
            libtemp = pc + 2 + 3 + 4 * 5;
         else
            libtemp = pc + 2 * libidsize + 4 + 3 + 4 * 5;
         add_auto_format( libtemp, T_FMT_RPL );
      }
   }

   sprintf( tempname, "LibEnd%03X", libnum );
   add_local_name( ( libend & 0xFFFFF ), tempname, T_SYM_LNAME );
   if( !opt_source ) {
      OUTREL5( SYMB_ABS( ( libend & 0xFFFFF ) ) );
      comment( "Library size" );
      print_line();
   }
     
   /* Print title size */
   new_instr();
   libidsize = get_unibbles( 2 );
   if( !opt_source ) {
      OUT2( "CON(2)\t#%X", libidsize );
      comment( "Library title size" );
      print_line();
   } else if( libidsize ) {
      mode_fixed_line = TRUE;
      fixed_ops = 1;
      OUT1( "xTITLE" );
      print_line();
   }
     
   /* Print library title */
   if( libidsize ) {
      new_instr();
      if( !opt_source )
	 OUT2( "NIBASC\t'%s'", make_ascstr( libidsize, HPCHR, YESSPC ) );
      else
	 OUT2( "%s", make_ascstr( libidsize, HPCHR, NOSPC ) );
      print_line();

      /* Printing library title size again */
      new_instr();
      libidsize = get_unibbles( 2 );
      if( !opt_source ) {
	 OUT2( "CON(2)\t#%X", libidsize );
	 comment( "Library title size again" );
	 print_line();
      }
   }

   /* Print library number */
   new_instr();
   libnum = get_unibbles( 3 );
   if( !opt_source ) {
      OUT2( "CON(3)\t#%X", libnum );
      comment( "Library number %%%d", libnum );
   } else
      OUT3( "xROMID\t%X  ( %%%d )", libnum, libnum );
   print_line();
   print_source();
     
   /* Print & mark hash table */
   new_instr();
   hashoff = get_unibbles( 5 );
   if( hashoff )
      hashloc = pc + hashoff - 5 ;
   else
      hashloc = 0;
   if( hashoff ) {
      if( ( pass == PASS1 ) &&
          ( opt_startpt <= hashloc ) && ( hashloc <= opt_endpt ) )
	 add_auto_format( hashloc, T_FMT_HASH );
      sprintf( tempname, "LibHash%03X", libnum );
      add_local_name( hashloc, tempname, T_SYM_LNAME );
      OUTREL5( SYMB_ABS( hashloc ) );
   } else
	  OUTCON0();
   comment( "Hash table offset" );
   if( !opt_source )
	  print_line();


   /* Print & mark message table */
   new_instr();
   comment( "Message table offset" );
   mesgoff = get_unibbles( 5 );   
   mesgloc = ( pc + mesgoff - 5 ) & 0xFFFFF;
   if( mesgoff ) {
      if( ( pass == PASS1 ) &&
          ( opt_startpt <= mesgloc ) && ( mesgloc <= opt_endpt ) )
         add_auto_format( mesgloc, T_FMT_RPL );
      sprintf( tempname, "LibMsg%03X", libnum );
      add_local_name( mesgloc, tempname, T_SYM_LNAME );
      if( !opt_source )
	 OUTREL5( SYMB_ABS( mesgloc ) );
      else
	 OUT2( "xMESSAGE\t%s", tempname );
      print_line();
      print_source();
   } else if( !opt_source ) {
      OUTCON0();
      print_line();
   }
     
   /* Print & mark link table */
   new_instr();
   linkoff = get_unibbles( 5 );
   linkloc = pc + linkoff - 5 ;
   if( linkoff ) {
      if( ( pass == PASS1 ) &&
          ( opt_startpt <= linkloc ) && ( linkloc <= opt_endpt ) )
	 add_auto_format( linkloc, T_FMT_LINK );
      sprintf( tempname, "LibLink%03X", libnum );
      add_local_name( linkloc, tempname, T_SYM_LNAME );
      OUTREL5( SYMB_ABS( linkloc ) );
   } else
      OUTCON0();
   comment( "Link table offset" );
   if( !opt_source )
      print_line();
     
   /* Print & mark config code */
   new_instr();
   comment( "Config code offset" );
   cfgoff = get_unibbles( 5 );
   cfgloc = pc + cfgoff - 5;
   if( cfgoff ) {
      if( ( pass == PASS1 ) &&
          ( opt_startpt <= cfgloc ) && ( cfgloc <= opt_endpt ) )
	 add_auto_format( cfgloc, T_FMT_RPL );
      sprintf( tempname, "LibCfg%03X", libnum );
      add_local_name( ( cfgloc & 0xFFFFF), tempname, T_SYM_LNAME );
      if( !opt_source )
	 OUTREL5( SYMB_ABS( cfgloc & 0xFFFFF ) );
      else
	 OUT2( "xCONFIG\t%s", tempname );
      print_line();
      print_source();
   } else if( !opt_source ) {
      OUTCON0();
      print_line();
   }

     if(pass==PASS2 & !opt_source & !mode_rpl)
	  printf("RPL\n");
     mode_rpl=new_mode_rpl=TRUE;

     /* Mark romptrs listed in link table */
     if (linkoff)
      {
	   static char hashname[20];
	   int old_pc, link_size, pointee, rompnum;
	   int link_off;

	   if((pass == PASS2) && (opt_code | opt_symdef))
	    {
		 separate(TRUE);
		 printf("* ROMPTR table\n");
		 separate(TRUE);
	    }
	   rompnum=0;
	   old_pc=pc;
	   set_pc(linkloc+5);
	   link_size=get_pcunibbles(5)-5;
	   /* Scanning tables */
	   while((link_size > 0) && (pc < libend ) )
	    {
		 link_off = get_pcnibbles(5);
		 if(link_off)
		  {
		       pointee=pc + link_off - 5;
		       sprintf(hashname,hash_name(hashloc,rompnum));
		       add_local_name(pointee,hashname, T_SYM_ROMP);
		       add_local_name(0x1000000+libnum*0x1000+rompnum,
				      hashname, T_SYM_LNAME);
		       /* Add format */
		       if(pass==PASS1)
			    add_auto_format(pointee, T_FMT_RPL);
		       if((pass == PASS2) && (opt_code | opt_symdef))
			  printf("EXTERNAL %s\t%s( ROMPTR %X %X\tat %05X )\n",
				 hashname,
				 strlen(hashname) < 7 ? "\t" : "",
				 libnum, rompnum, pointee);
		  }
		 /* Next ROMPTR */
		 link_size -= 5;
		 rompnum++ ;
	    }
	   set_pc(old_pc);
	   if((pass == PASS2) && (opt_code | opt_symdef))
		separate(TRUE);
      }
     
#ifdef 0 /* when this is active a lot of labels aren't generated, FIXME */
   if( pass == PASS1 ) {
      /* Only library header & links caused auto formats so far */
      add_autos_to_fmttab();
      nextfmt = fmtref;
      for(; nextfmt && *nextfmt && ((*nextfmt)->val <= pc); nextfmt++);
   }
#endif
}


/* Disassemble library without a prolog */
void decode_romlibrary()
{
     disass_library(FALSE, FALSE);
}

/* Disassemble CRC */
void decode_crc()
{
     int crc;
     
     new_instr();
     crc = get_unibbles(4);
     
     if(opt_source)
	  return;
     
     new_mode_rpl = FALSE;
     
     OUT2("CON(4)\t#%04X", crc);
     comment("4 nibble CRC");
     print_line();
}


/* ********************************************************************** */
/* *               Grob disassembling                                   * */
/* ********************************************************************** */

static void disass_grob(rpl)
    int rpl;
{
     int grob_size, grob_rows,grob_columns;
     int nibs_on_row,row,col,bit,grob_nib;

     new_mode_rpl=FALSE;
     if(rpl)
      {
	   OUTCON5(SYMB_ABS(GROB));
	   print_line();
	   new_instr();
      }
     grob_size=get_unibbles(5);
     OUTREL5(SYMB_ABS(pc+grob_size-5));
     comment("Grob size");
     print_line();
     
     new_instr();
     grob_rows=get_unibbles(5);
     OUT2("CON(5)\t#%X",grob_rows);
     comment("Grob rows");
     print_line();

     new_instr();
     grob_columns=get_unibbles(5);
     OUT2("CON(5)\t#%X",grob_columns);
     comment("Grob columns");
     print_line();

     if(grob_rows)
	  nibs_on_row=(grob_size-15)/grob_rows;
     else
	  nibs_on_row=0;

     /* Sometimes grobs are filled in by expanding */
     /* a grob with the wanted x and y sizes */
     /* This check isn't accurate */
     if((grob_size-15) < (grob_rows * (grob_columns)/4))
	  grob_rows=0;

     /* Print grob body */
     if(grob_rows)
      {
	   if(opt_source)
		opt_comments = TRUE;
	   for(row=0 ; row < grob_rows ; row++ )
	    {
		 new_instr();
		 OUTNIBHEX(nibs_on_row);
		 if(opt_drawgrob & !opt_groblast)
		  {
		       set_pc(pc-nibs_on_row);
		       if(nibs_on_row < 8 )
			    comment("|%s|", make_grobstr(nibs_on_row));
		       else if (nibs_on_row < 23)
			{
			     comment("|%s..|", make_grobstr(8));
			     set_pc(pc+nibs_on_row-8);
			}
		       /* Else discard too wide grob */
		       else
			    set_pc(pc+nibs_on_row);
		  }
		 print_line();
	    }
	   if(opt_source)
		opt_comments = FALSE;
	   if(opt_drawgrob & opt_groblast & (pass==PASS2))
	    {
		 set_pc(pc-grob_rows*nibs_on_row);
		 if(nibs_on_row>17)
		      for(row=0 ; row < grob_rows ; row++ )
		       {
			    printf("* |%s...| *\n", make_grobstr(17));
			    set_pc(pc+nibs_on_row-17);
		       }
		 else
		      for(row=0 ; row < grob_rows ; row++ )
			   printf("* |%s| *\n", make_grobstr(nibs_on_row));
	    }
      }
}

/* ********************************************************************** */
/* *               Directory disassembling                              * */
/* ********************************************************************** */

static void disass_dir(rpl)
    int rpl;
{
     int off_to_last,cur_off;
     
     new_mode_rpl=FALSE;
     if(rpl)
      {
	   OUTCON5(SYMB_ABS(DIRECTORY));
	   print_line();
	   new_instr();
      }
     OUT2("CON(3)\t#%X", get_unibbles(3));
     comment("Attached library");
     print_line();
     
     new_instr();
     off_to_last=get_unibbles(5);
     if(off_to_last)
	  OUTREL5(SYMB_ABS(pc+off_to_last-5));
     else
	  OUTCON0();
     print_line();
     
     if(off_to_last)
      {
	   set_pc(pc+off_to_last-10);
	   while(cur_off = get_pcunibbles(5))
	    {
		 if(pass==PASS1)
		      add_auto_format(pc-5, T_FMT_VAR);
		 set_pc(pc-10-cur_off);
	    }
	   if(pass==PASS1)
		add_auto_format(pc-5, T_FMT_VAR);
	   set_pc(pc-5);
	   if(pass==PASS1)
	    {
		 add_autos_to_fmttab();
		 nextfmt = fmtref;
		 for(; nextfmt && *nextfmt && ((*nextfmt)->val <= pc); nextfmt++);
	    }
      }
}

/* ********************************************************************** */
/* *               Tagged disassembling                                 * */
/* ********************************************************************** */

static void disass_tagged(rpl)
    int rpl;
{
     int len = get_unibbles(2);

     if(rpl)
      {
	   if(len)
		OUT2("TAG %s", make_ascstr(len,HPCHR,NOSPC));
	   else
		OUT1("TAG");
	   print_line();
      }
     else
      {
	   OUTCON5(SYMB_ABS(TAGGED));
	   print_line();
	   new_instr();
	   OUT2("CON(2)\t#%X", len);
	   print_line();
	   if(len)
	    {
		 new_instr();
		 OUT2("NIBASC\t'%s'", make_ascstr(len, HPCHR,YESSPC));
		 print_line();
	    }
      }
}


/* ********************************************************************** */
/* *               Array disassembling                                  * */
/* ********************************************************************** */

static void disass_arry(rpl, linkedp)
    int rpl;
{
     int arry_size, arry_end, prolog, elements, dims, len, offset, temp;
     
     if(!opt_source)
	  new_mode_rpl=FALSE;

     if(rpl)
      {
	   if(!linkedp && opt_source)
	    {
		 print_source();
		 OUT1("ARRY");
	    }
	   else
		OUTCON5(SYMB_ABS(linkedp ? LINKED_ARRAY : ARRAY));
	   print_line();
	   new_instr();
      }
     arry_size = get_unibbles(5);
     arry_end = pc +arry_size-5;
     
     if(linkedp || !opt_source)
      {
	   OUTREL5(SYMB_ABS(arry_end));
	   comment("Array size");
	   print_line();
	   new_instr();
      }
     
     prolog = get_unibbles(5);
     if(linkedp || !opt_source)
      {
	   OUTCON5(SYMB_ABS(prolog));
	   print_line();
	   new_instr();
      }
     dims = get_unibbles(5);
     if(linkedp || !opt_source)
      {
	   OUT2("CON(5)\t%d", dims);
	   comment("Dimensions");
	   print_line();
      }
     
     elements = 1;
     while(dims--)
      {
	   new_instr();
	   len = get_unibbles(5);
	   if(linkedp || !opt_source)
		OUT2("CON(5)\t%d", len);
	   else
		OUT2("%X", len);
	   print_line();
	   elements *= len;
      }

     if(!linkedp && opt_source)
      {
	   rpl_comp_level++;
	   rpl_new_ind_level++;
	   new_instr();
	   OUT1("[");
	   print_line();
	   print_source();
      }



     temp = elements;
     if(linkedp)
	  while((temp--) && (pc < opt_endpt))
	   {
		new_instr();
		offset = get_nibbles(5);
		OUTREL5(SYMB_ABS(pc+offset-5));
		comment("Offset to element %d", elements-temp);
		print_line();
	   }


     /* To get comments for the key-press arrays: */
     if(!linkedp & opt_source & (prolog==SYSTEM_BINARY))
	  opt_comments=TRUE;

     while((elements--) && (pc < arry_end) && (pc < opt_endpt))
      {
	  /* Disassemble body only */
	   new_instr();
	   if(linkedp || !opt_source)
		disass_body(prolog, FALSE);
	   else
	    {
		 if(prolog==SYSTEM_BINARY)
		      mode_single_op = TRUE;
		 disass_body(prolog, TRUE);
		 print_source();
		 if(elements==0)
		  {
		       rpl_comp_level--;
		       rpl_new_ind_level--;
		       mode_single_op = TRUE;
		       OUT1("]");
		       print_line();
		  }
	    }
      }
     /* Resuming normal source mode */
     if(!linkedp & opt_source & (prolog==SYSTEM_BINARY))
	  opt_comments=FALSE;
}


/* ********************************************************************** */
/* *               SEMI disassembling                                   * */
/* ********************************************************************** */

/* SEMI */
static void disass_semi(rpl)
    int rpl;
{
     mode_single_op = TRUE;

     if(rpl_comp_level>0)
      {
	   rpl_comp_level--;
	   rpl_ind_level--;
	   rpl_new_ind_level=rpl_ind_level;
      }
     if(rpl)
	  OUT1(";");
     else
	  OUTCON5(SYMB_ABS(SEMI));
     print_line();
}

/* ********************************************************************** */
/* *               Composite disassembling                              * */
/* ********************************************************************** */

/* Composite objects */
static void disass_composite(prolog, prname, rpl)
    int prolog, rpl;
    char *prname;
{
     if(mode_imaginary_composite)
      {
	   mode_imaginary_composite = FALSE;
	   rpl_ind_level--;
	   rpl_new_ind_level--;
	   rpl_comp_level--;
      }

     if(mode_dispatch)
	  mode_fixed_line = FALSE;
     mode_single_op = TRUE;
     if(rpl)
      {
	   OUT1(prname);
	   rpl_comp_level++;
	   rpl_new_ind_level++;
      }
     else
	  OUTCON5(SYMB_ABS(prolog));
     print_line();
}

/* ********************************************************************** */
/* *               System binary disassembling                          * */
/* ********************************************************************** */

static void disass_system_binary(rpl)
    int rpl;
{
     int body = get_unibbles(5);
     if(rpl)
      {
	   OUT2("# %X", body);
	   if(body>=0x100)
		comment("%s", SYMB_RPL(body));
      }
     else
	  OUTCON5(SYMB_ABS(body));
     print_line();
}


/* ********************************************************************** */
/* *               Real number disassembling                            * */
/* ********************************************************************** */

static void disass_real(rpl)
    int rpl;
{
     hp_real r;
     static char realbuf[132];
     r.x = get_unibbles(3);
     r.mr= get_unibbles(3);
     r.m = get_unibbles(8);
     r.m1= get_unibbles(1);
     r.s = get_unibbles(1);
     if(rpl)
	  OUT2("%% %s", realtos(&r, realbuf));
     else
      {
	   set_pc(pc-16);
	   new_instr();
	   OUTNIBHEX(16);
	   comment("[%%] %s", realtos(&r, realbuf));
      }
     print_line();
}


/* ********************************************************************** */
/* *               Complex number disassembling                         * */
/* ********************************************************************** */

static void disass_complex(rpl)
    int rpl;
{
     hp_real re;
     static char rebuf[132];
     hp_real im;
     static char imbuf[132];
     
     re.x = get_unibbles(3);
     re.mr= get_unibbles(3);
     re.m = get_unibbles(8);
     re.m1= get_unibbles(1);
     re.s = get_unibbles(1);
     im.x = get_unibbles(3);
     im.mr= get_unibbles(3);
     im.m = get_unibbles(8);
     im.m1= get_unibbles(1);
     im.s = get_unibbles(1);

     if(rpl)
	  OUT3("C%% %s %s", realtos(&re,rebuf),realtos(&im,imbuf));
     else
      {
	   set_pc(pc-32);
	   new_instr();
	   OUTNIBHEX(16);
	   comment("[C%%re] %s", realtos(&re, rebuf));
	   print_line();
	   new_instr();
	   OUTNIBHEX(16);
	   comment("[C%%im] %s", realtos(&im, imbuf));
      }
     print_line();
}


/* ********************************************************************** */
/* *               Long real number disassembling                       * */
/* ********************************************************************** */

static void disass_long_real(rpl)
    int rpl;
{
     hp_longreal lr;
     static char lrealbuf[132];
     lr.x = get_unibbles(5);
     lr.mr= get_unibbles(6);
     lr.m = get_unibbles(8);
     lr.m1= get_unibbles(1);
     lr.s = get_unibbles(1);
     if(rpl)
	  OUT2("%%%% %s",longrealtos(&lr, lrealbuf));
     else
      {
	   set_pc(pc-21);
	   new_instr();
	   OUTNIBHEX(21);
	   comment("[%%%%] %s", longrealtos(&lr, lrealbuf));
      }
     print_line();
}


/* ********************************************************************** */
/* *               Long complex number disassembling                    * */
/* ********************************************************************** */

static void disass_long_complex(rpl)
    int rpl;
{
     static char rebuf[132];
     static char imbuf[132];
     hp_longreal re;
     hp_longreal im;

     re.x = get_unibbles(5);
     re.mr= get_unibbles(6);
     re.m = get_unibbles(8);
     re.m1= get_unibbles(1);
     re.s = get_unibbles(1);
     im.x = get_unibbles(5);
     im.mr= get_unibbles(6);
     im.m = get_unibbles(8);
     im.m1= get_unibbles(1);
     im.s = get_unibbles(1);

     if(rpl)
      {
	   /* Local (new) alpha station has trouble dealing both calls at once
	    * so I separated them. Strangely enough disass_complex works as is
	    *
	    *  sprintf(l_instr, "C%%%% %s %s",
	    *          longrealtos(&re, rebuf), longrealtos(&im, imbuf));
	    */

	   char * l_instrp;
	   l_instrp = l_instr;
	   sprintf(l_instrp, "C%%%% %s", longrealtos(&re, rebuf));
	   l_instrp += strlen(l_instrp);
	   sprintf(l_instrp, " %s", longrealtos(&im, imbuf));
      }
     else
      {
	   set_pc(pc-42);
	   new_instr();
	   OUTNIBHEX(21);
	   comment("[C%%%%re] %s", longrealtos(&re, rebuf));
	   print_line();
	   new_instr();
	   OUTNIBHEX(21);
	   comment("[C%%%%im] %s", longrealtos(&im, imbuf));
      }
     print_line();
}

/* ********************************************************************** */
/* *               Character disassembling                              * */
/* ********************************************************************** */

static void disass_character(rpl)
    int rpl;
{
     if(rpl)
	  OUT2("CHR %s", make_ascstr(1,HPCHR,NOSPC));
     else
	  OUT2("NIBASC\t'%s'",make_ascstr(1,HPCHR,YESSPC));
     print_line();
}

/* ********************************************************************** */
/* *               String disassembling                                 * */
/* ********************************************************************** */

static void disass_string(rpl)
    int rpl;
{
     if(rpl)
      {
	   int len = get_unibbles(5);
	   
	   if(len==5)
		OUT1("$ \"\"");
	   else if((len < 80) && (len & 1))
		OUT2("$ \"%s\"", make_ascstr((len-5)/2,HPCHR,YESSPC));
	   else if ((len > 5) && (len & 1))
	    {
		 new_mode_rpl=FALSE;
		 len-=5;
		 OUTCON5(SYMB_ABS(STRING));
		 print_line();
		 new_instr();
		 OUTREL5(SYMB_ABS(pc+len));
		 while(len>0)
		  {
		       print_line();
		       new_instr();
		       if(len >= 80)
			    OUT2("NIBASC\t'%s'",make_strline(STRLINEMAX,HPCHR));
		       else
			    OUT2("NIBASC\t'%s'",make_strline(len/2, HPCHR));
		       len -= pc - org_pc;
		  }
	    }
	   else
	    {
		 new_mode_rpl=FALSE;
		 set_pc(pc-10);
		 OUTNIBHEX(10);
		 comment("Invalid length");
	    }
	   print_line();
      }
     else
	  disass_bodyasc(STRING, 5, FALSE, TRUE);
}

/* ********************************************************************** */
/* *               Hex string disassembling                             * */
/* ********************************************************************** */

static void disass_binary_int(rpl)
    int rpl;
{
     if(rpl)
      {
	   int len = get_pcunibbles(5);
	   set_pc(pc-5);
	   if(len>5)
		if(len<40)
		 {
		      len = get_unibbles(5);
		      OUT3("HXS %X %s", len-5, make_hexstr(len-5));
		      print_line();
		 }
		else
		     disass_bodyhex(BINARY_INT, 5, TRUE, TRUE);
	   else
	    {
		 new_mode_rpl=FALSE;
		 set_pc(pc-5);
		 new_instr();
		 OUTNIBHEX(10);
		 comment("Null hex");
		 print_line();
	    }
      }
     else
	  disass_bodyhex(BINARY_INT, 5, FALSE, TRUE);
}

/* ********************************************************************** */
/* *               Long integer disassembling                           * */
/* ********************************************************************** */

static void disass_long_int(rpl)
    int rpl;
{
     if(rpl)
      {
	   int len = get_pcunibbles(5);
	   set_pc(pc-5);
	   if(len>5)
		if(len<40)
		 {
		      len = get_unibbles(5);
		      OUT2("ZINT %s", make_hexstr(len-5));
		      print_line();
		 }
		else
		     disass_bodyhex(LONG_INT, 5, TRUE, TRUE);
	   else
	    {
		 new_mode_rpl=FALSE;
		 set_pc(pc-5);
		 new_instr();
		 OUTNIBHEX(10);
		 comment("Null int");
		 print_line();
	    }
      }
     else
	  disass_bodyhex(LONG_INT, 5, FALSE, TRUE);
}

/* ********************************************************************** */
/* *               Global name disassembling                            * */
/* ********************************************************************** */

static void disass_global_name(rpl)
    int rpl;
{
     if(rpl)
      {
	   int len = get_unibbles(2);
	   if(len)
		OUT2("ID %s", make_ascstr(len,HPCHR,NOSPC));
	   else
	    {
		 new_mode_rpl=FALSE;
		 set_pc(pc-7);
		 OUTNIBHEX(7);
		 comment("Null ID");
	    }
	   print_line();
      }
     else
	  disass_bodyasc(GLOBAL_NAME, 2, FALSE, FALSE);
}

/* ********************************************************************** */
/* *               Local name disassembling                             * */
/* ********************************************************************** */

static void disass_local_name(rpl)
    int rpl;
{
     if(rpl)
      {
	   int len = get_unibbles(2);
	   if(len)
		OUT2("LAM %s", make_ascstr(len,HPCHR,NOSPC));
	   else
	    {
		 new_mode_rpl=FALSE;
		 set_pc(pc-7);
		 OUTNIBHEX(7);
		 comment("Null LAM");
	    }
	   print_line();
      }
     else
	  disass_bodyasc(LOCAL_NAME, 2, FALSE, FALSE);
}

/* ********************************************************************** */
/* *               Romptr disassembling                                 * */
/* ********************************************************************** */

static void disass_romptr(rpl)
    int rpl;
{
     int rlib, rcmd;

     rlib = get_unibbles(3);
     rcmd = get_unibbles(3);
     if(rpl)
      {
	   if(!opt_source | (rlib != libnum))
	    {
		 OUT3("ROMPTR %X %X", rlib, rcmd);
		 comment("%s",symbolic(0x1000000+rlib*0x1000+rcmd,
				       NOREL,SRPL));
	    }
	   else
		OUT1(hash_name(hashloc, rcmd));
      }
     else
      {
	   OUT2("CON(6)\t#%X", rlib*0x1000+rcmd);
	   comment("%s",symbolic(0x1000000+rlib*0x1000+rcmd, NOREL,SRPL));
      }
     print_line();
}

/* ********************************************************************** */
/* *               Flashptr disassembling                               * */
/* ********************************************************************** */

static void disass_flashptr(rpl)
    int rpl;
{
     int rlib, rcmd;

     rlib = get_unibbles(3);
     rcmd = get_unibbles(4);
     if(rpl)
      {
	   OUT3("FPTR %X %X", rlib, rcmd);
	   comment("%s",symbolic(0x1000000+rlib*0x1000+rcmd,
		       NOREL,SRPL));
      }
     else
      {
	   OUT2("CON(7)\t#%X", rlib*0x1000+rcmd);
	   comment("%s",symbolic(0x1000000+rlib*0x1000+rcmd, NOREL,SRPL));
      }
     print_line();
}


/* ********************************************************************** */
/* *               Code disassembling                                   * */
/* ********************************************************************** */

static void disass_code( int rpl )
{
   int len = get_unibbles( 5 );

   mode_single_op = TRUE;
   rpl_new_ind_level++;
   if( rpl ) {
      OUT1( "CODE" );
      comment( "Lenght: #%X End:#%X", len - 5, pc + len - 5 );
   } else
      OUTREL5( SYMB_ABS( pc + len - 5 ) );
   print_line();
   
   if( len > 5 ) {
      /* Sometimes people edit null length code objects for protection */
      /* So we don't expire them */
      prevfmt = curfmt;
      fmtexpire_tag = pc + len - 5 ;
   }
   mode_rpl = new_mode_rpl = FALSE;
   format_code();
}


/* ********************************************************************** */
/* *               ACPTR disassembling                                  * */
/* ********************************************************************** */

static void disass_acptr(rpl)
    int rpl;
{
     int addr, access;

     new_mode_rpl=FALSE;
     addr   = get_unibbles(5);
     access = get_unibbles(5);
     if(rpl)
      {
	   OUT3("ACPTR\t#%X %s", addr, SYMB_ABS(access));
	   print_line();
	   new_instr();
      }
     else
      {

	     OUTCON5(SYMB_ABS(addr));
	     comment("Data location");
	     print_line();

	     new_instr();
	     if(access==0)
		  OUTCON0();
	     else
		  OUTCON5(SYMB_ABS(access));
	     comment("Access routine");
	     print_line();
      }
}


/* ********************************************************************** */
/* *               RPL object disassembling                             * */
/* ********************************************************************** */

/* If rpl = TRUE rpl disassembly format is used, else data format */
static void disass_body(prolog, rpl)
    int prolog, rpl;
{
     
     if(rpl)
	  new_mode_rpl=TRUE;
     else
	  new_mode_rpl=FALSE;

     /* First see if special LINK */
     if(prolog == pc)
      {
	   /* Assuming PCO's are not embedded in composites */
	   rpl_comp_level = 0;
	   rpl_ind_level = 0;
	   rpl_new_ind_level = 0;

	   if(rpl)
	    {
		 format_code();
		 OUT1("CON(5)\t(*)+5");
		 comment("*PRIMITIVE*");
	    }
	   else
		OUTCON5(symbolic(prolog,NORELRPL,SCODE));
	   print_line();
	   return;
      }

     if(rpl & (rpl_comp_level==0) & (prolog != ARRAY))
      {
	   mode_imaginary_composite = TRUE;
	   rpl_comp_level++;
	   rpl_ind_level++;
	   rpl_new_ind_level++;
      }

     switch(prolog)
      {
      case SEMI:
	   disass_semi(rpl);
	   break;
      case SYSTEM_BINARY:
	   disass_system_binary(rpl);
	   break;
      case REAL_NUMBER:
	   disass_real(rpl);
	   break;
      case LONG_REAL:
	   disass_long_real(rpl);
	   break;
      case COMPLEX:
	   disass_complex(rpl);
	   break;
      case LONG_COMPLEX:
	   disass_long_complex(rpl);
	   break;
      case CHARACTER:
	   disass_character(rpl);
	   break;
      case STRING:
	   disass_string(rpl);
	   break;
      case BINARY_INT:
	   disass_binary_int(rpl);
	   break;
      case LONG_INT:
	   disass_long_int(rpl);
	   break;
      case GLOBAL_NAME:
	   disass_global_name(rpl);
	   break;
      case LOCAL_NAME:
	   disass_local_name(rpl);
	   break;
      case ROMPTR:
	   disass_romptr(rpl);
	   break;
      case FLASHPTR:
	   disass_flashptr(rpl);
	   break;
      case LIST:
	   disass_composite(prolog,"{",rpl);
	   break;
      case ALGEBRAIC:
	   disass_composite(prolog,"SYMBOL",rpl);
	   break;
      case UNIT:
	   disass_composite(prolog,"UNIT",rpl);
	   break;
      case TAGGED:
	   disass_tagged(rpl);
	   break;
      case PROGRAM:
	   disass_composite(prolog,"::",rpl);
	   break;
      case CODE:
	   disass_code(rpl);
	   break;
      case DIRECTORY:
	   disass_dir(rpl);
	   break;
      case ARRAY:
	   disass_arry(rpl, FALSE);
	   break;
      case LINKED_ARRAY:
	   disass_arry(rpl, TRUE);
	   break;
      case GROB:
	   disass_grob(rpl);
	   break;
      case EXT1:
	   if(opt_gx || opt_hp49g)
		disass_acptr(rpl);
	   else
		disass_bodyhex(prolog,5,rpl,TRUE);
	   break;
      case BACKUP:
      case LIBRARY_DATA:
      case EXT2:
      case EXT3:
	   disass_bodyhex(prolog,5,rpl,TRUE);
	   break;
      case LIBRARY:
	   disass_library(rpl, TRUE);
	   break;
      default:
	   if( (opt_startpt<=prolog) && (prolog<=opt_endpt) )
		add_local_symbol(prolog, T_SYM_NOHP);
	   if(rpl)
		OUT1(SYMB_RPL(prolog));
	   else
		OUTCON5(symbolic(prolog, NORELRPL, SCODE));

	   /* Add format */
	   if(opt_formats && pass == PASSF)
		add_auto_format(prolog, T_FMT_RPL);
	   print_line();
      }
}

/* ********************************************************************** */
/* *               Disassembling RPL objects                            * */
/* ********************************************************************** */

/* Decode RPL code */
void decode_rpl()
{
     int tag, propsize;
     char *s;
     struct symbol *mdef;
     
     rpl_new_ind_level = rpl_ind_level;
     new_instr();

     if((propsize=is_header())!=0)
      {
	   decode_header(propsize);
	   mode_rpl=new_mode_rpl=TRUE;
	   if(pass==PASS2 & opt_code)
		printf("RPL\n");
	   return;
      }
    
     new_instr();
     if(!opt_entry & (pc > opt_endpt-5))
      {
	   int nibs = opt_endpt-pc;
	   int lastnibs = get_unibbles(nibs);
	   
	   new_mode_rpl=FALSE;
	   if(!opt_source & (libnum!=0))
	    {
		 if(opt_endpt > pc)
		      OUT3("CON(%d)\t#%X", nibs, lastnibs);
		 comment("Sync at end");
		 print_line();
	    }
	   return;
      }

     tag = get_unibbles(5);

     if(tag==0)
      {
	   /* Disassemble stream of zeros */

	   int zerlen = 5;
	   new_mode_rpl = FALSE;

	   while(get_pcunibbles(1)==0)
		zerlen++;

	   set_pc(pc-zerlen-1);
	   new_instr();
	   (void) make_hexstr(zerlen);		/* Needed for PC field */
	   OUT2("BSS\t#%X", zerlen);
	   print_line();

	   new_mode_rpl=TRUE;
      }

     /* See if we have a suitable macro definition */
     else if((mdef = macro(5, tag)) && (s = macro_expand(mdef)))
      {
	   OUT1(s);
	   /* Using no format on purpose to get only address comment */
	   comment("", symbolic(tag, NOREL, SRPL));
	   /* Add format */
	   if(opt_formats && pass == PASSF)
		add_auto_format(tag, T_FMT_RPL);
	   print_line();
      }
     else
      {
	   /* Disassemble body as RPL */
	   disass_body(tag, TRUE);
      }
}

/* ********************************************************************** */
/* *               Data disassembling                                   * */
/* ********************************************************************** */

/* Jumping around... */

void decode_data();

/* Decode data object once */
static void decode_dataob(wid,type)
    int wid,type;
{
     int wid2, rel;

     new_mode_rpl=FALSE;
     switch(type)
      {
      case FT_INSTR1:
	   decode_instr();
	   print_line();
	   break;
      case FT_RPL1:
	   disass_body(get_unibbles(5),TRUE);
	   break;
      case FT_REL:
	   rel=get_unibbles(wid);
	   if(rel)
		OUT3( "REL(%d)\t%s",
                      wid,
                      SYMB_ABS( get_relative( pc + rel - wid, wid ) ) );
	   else
		OUT2("CON(%d)\t0", wid);
	   FORMAT(pc+rel-wid, T_FMT_CODE);
	   print_line();
	   break;
      case FT_HEX:
	   if(wid == 5)
		OUTCON5(CKSYMB_ABS(get_unibbles(wid)));
	   else if (wid < 5)
		OUT3("CON(%d)\t#%X", wid, get_unibbles(wid));
	   else
		OUTNIBHEX(wid);
	   print_line();
	   break;
      case FT_VHEX:
	   disass_bodyhex(0,wid,FALSE,TRUE);
	   break;
      case FT_VHEX2:
	   disass_bodyhex(0,wid,FALSE,FALSE);
	   break;
      case FT_SPCHAR:
	   OUTNIBHEX(wid*2);
	   set_pc(pc-2*wid);
	   comment("'%s'", make_ascstr(wid, SPCHR, YESSPC));
	   print_line();
	   break;
      case FT_STR:
	   OUT2("NIBASC\t'%s'", make_ascstr(wid,HPCHR,YESSPC));
	   print_line();
	   break;
      case FT_STRLINE:
	   OUT2("NIBASC\t'%s'", make_strline(wid,HPCHR));
	   print_line();
	   break;
      case FT_VSTR:
	   disass_bodyasc(0,wid,FALSE,TRUE);
	   break;		      
      case FT_VSTR2:
	   disass_bodyasc(0,wid,FALSE,FALSE);
	   break;
      case FT_DEC:
	   OUT3("CON(%d)\t%d", wid,get_unibbles(wid));
	   print_line();
	   break;
      case FT_OCT:
	   OUT3("CON(%d)\t#%Oo",wid,get_unibbles(wid));
	   print_line();
	   break;
      case FT_BIN:
	   OUT3("CON(%d)\t#%ub",wid,get_unibbles(wid));
	   print_line();
	   break;
      case FT_FLOAT:
	   disass_real(FALSE);
	   break;
      case FT_GRBROW:
	   OUTNIBHEX(wid);
	   if(opt_drawgrob)
		if(opt_source)
		 {
		      set_pc(pc-wid);
		      opt_comments = TRUE;
		      if(wid < 8 )
			   comment("|%s|",make_grobstr(wid));
		      else
		       {
			    comment("|%s..", make_grobstr(8));
			    set_pc(pc+wid-8);
		       }
		      print_line();
		      opt_comments = FALSE;
		 }
		else
		 {
		      int tempflag;
		      tempflag = opt_comments;
		      opt_comments = TRUE;
		      set_pc(pc-wid);
		      comment("%s", make_grobstr(wid));
		      print_line();
		      opt_comments = tempflag;
		 }
	   else
		print_line();	/* Outputs the plain NIBHEX */
	   break;
      case FT_CRC:
	   decode_crc();
	   break;
      default:
	   OUTNIBHEX(wid);
	   comment("Overrode invalid format: %X", type);
	   print_line();
	   break;
      }
}

/* Decode according to data format entry */
static void decode_data_ent(fent)
    struct format_ent *fent;
{
     int rept;
     
     for(rept = fent->repeat; rept > 0; rept--)
	  if(fent->ftype & FT_SUBD)
	       decode_data(fent->fmtdescr);
	  else
	   {
		new_instr();
		decode_dataob(fent->width, fent->ftype & FT_DATA_MASK);
	   }
     branch=FALSE;
     /* If grobdata has repeat count then separate rows */
     if( ((fent->ftype & FT_DATA_MASK) == FT_GRBROW) & (fent->repeat > 1))
	  if( (pass==PASS2) & opt_code)
	       putchar('\n');
}


/* Decode data according to format */
void decode_data(fmt)
    struct format *fmt;
{
     struct format_ent *fent;
     
     branch=FALSE;	/* Prevents further incorrect disass */
     if(!fmt)
      {
	   new_mode_rpl=FALSE;
	   decode_instr();
	   print_line();
	   return;
      }
     for(fent = fmt->fmtspec; fent < fmt->fmtspec + fmt->nformats; fent++)
	  decode_data_ent(fent);
}
