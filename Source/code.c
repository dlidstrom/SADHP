/* code.c -- SAD machine code disassembler programs.

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
#include "globals.h"

static char     
     *fixed_0e[] = {
	   "A=A&B", "B=B&C", "C=C&A", "D=D&C",
	   "B=B&A", "C=C&B", "A=A&C", "C=C&D",
	   "A=A!B", "B=B!C", "C=C!A", "D=D!C",
	   "B=B!A", "C=C!B", "A=A!C", "C=C!D"  },
     
     *std_fields[] = {
	   "P", "WP", "XS", "X", "S", "M", "B", "W",
	   "P", "WP", "XS", "X", "S", "M", "B", "A" },
     
     *fixed_f1[] = {
	   "A", "A", "A", "A", "A", "A", "A", "A",
	   "C", "C", "C", "C", "C", "C", "C", "C"
	   },
     
     *fixed_rf1[] = {
	   "0", "1", "2", "3", "4", "1", "2", "3",
	   "0", "1", "2", "3", "4", "1", "2", "3"
	   };	

static void jumpify(loc, name, str)
    int loc;
    char *name, *str;
{
     static char nbuf[132];
     
     if(opt_jumpify)
      {
	   if(pass!=PASS1)
		return;
	   if(*name == '#')
		return;
	   if(strncmp(name, "L_",2)==0)
		name += 2;
	   else	if(*name == '=')
		name++;
	   sprintf(nbuf, "L_%s", name);
	   add_local_name(loc, nbuf, T_SYM_LNAME);
      }
     else
	  force_comment(str, name);
}


/* Does location contain RPL exit sequence? */
static int is_rplloop(loc)
    int loc;
{
     if((fetch_unibbles(loc+0,3)==0x241) &	/* A=DAT0 A */
	(fetch_unibbles(loc+3,3)==0x461) &	/* D0=D0+ 5 */
	(fetch_unibbles(loc+6,4)==0xC808))	/* PC=(A)   */
	  return(TRUE);
     else
	  return(FALSE);
}



/* Look ahead for jumping commands to comment possible new jumps */
/* Saves space not having to list all GOVLNG =SAVPTR etc commands in symbols */
static void comment_jump(loc)
    int loc;
{
     static char nbuf[132];

     int instr1, instr2, instr3, offs;
     
     if((loc < baseaddr) || (loc > coresize + baseaddr))
	  return;
     
     jumpaddr=loc;
     
     if(fetch_unibbles(loc,1)==0x6)		/* GOTO x */
      {
	   jumpaddr = (fetch_nibbles(loc+1,3)+loc+1) & 0xfffff;
	   jumpify(loc, SYMB_ABS(jumpaddr), "GOTO %s");
	   return;
      }

     switch(fetch_unibbles(loc,2))
      {
      case 0xC8:		/* GOLONG */
	   jumpaddr = (fetch_nibbles(loc+2,4)+loc+2) & 0xfffff;
	   jumpify(loc, SYMB_ABS(jumpaddr), "GOLONG %s");
	   return;
      case 0xD8:		/* GOVLNG */
	   jumpaddr = fetch_unibbles(loc+2,5);
	   jumpify(loc, symbolic(jumpaddr,NOREL,SRPL), "GOVLNG %s");
	   return;
      case 0x00:		/* RTNSXM */
	   jumpify(loc, "RTNSXM", "%s");
	   return;
      case 0x10:		/* RTN */
	   jumpify(loc, "RTN", "%s");
	   return;
      case 0x20:		/* RTNSC */
	   jumpify(loc, "RTNSC", "%s");
	   return;
      case 0x30:		/* RTNCC */
	   jumpify(loc, "RTNCC", "%s");
	   return;
      default:
	   break;
      }

     if(is_rplloop(loc))
      {
	   jumpify(loc, "LOOP", "%s");
	   return;
      }
     
     if(is_rplloop(loc+3))
	  switch(fetch_unibbles(loc,3))
	   {
	   case 0x031:				/* D0=A     */
		jumpify(loc, "D0=ALOOP", "%s");
		return;
	   case 0x131:				/* D1=A     */
		jumpify(loc, "D1=ALOOP", "%s");
		return;
	   case 0x231:				/* AD0EX    */
		jumpify(loc, "AD0EXLOOP", "%s");
		return;
	   case 0x331:				/* AD1EX    */
		jumpify(loc, "AD1EXLOOP", "%s");
		return;
	   case 0x431:				/* D0=C     */
		jumpify(loc, "D0=CLOOP", "%s");
		return;
	   case 0x551:				/* D1=C     */
		jumpify(loc, "D1=CLOOP", "%s");
		return;
	   case 0x631:				/* CD0EX    */
		jumpify(loc, "CD0EXLOOP", "%s");
		return;
	   case 0x731:				/* CD1EX    */
		jumpify(loc, "CD1EXLOOP", "%s");
		return;
	   case 0x141:				/* DAT1=A A */
		jumpify(loc, "OverWrALp", "%s");
		return;
	   case 0x441:				/* DAT1=C A */
		jumpify(loc, "OverWrCLp", "%s");
		return;
	   default:
		break;
	   }

     if(fetch_unibbles(loc,2)==0x80)		/* C=RSTK */
	  if(is_rplloop(loc+5))
	       switch(fetch_unibbles(loc+2,3))
		{
		case 0x431:				/* D0=C     */
		     jumpify(loc, "D0=RSTKLOOP", "%s");
		     return;
		case 0x551:				/* D1=C     */
		     jumpify(loc, "D1=RSTKLOOP", "%s");
		     return;
		case 0x631:				/* CD0EX    */
		     jumpify(loc, "RSTKD0EXLOOP", "%s");
		     return;
		case 0x731:				/* CD1EX    */
		     jumpify(loc, "RSTKD1EXLOOP", "%s");
		     return;
		case 0x441:				/* DAT1=C A */
		     jumpify(loc, "OverWrRstkLp", "%s");
		     return;
		default:
		     break;
	   }
     
     
     if((fetch_unibbles(loc,2)==0x43) &		/* LC(5) x  */
	(fetch_unibbles(loc+7,2)==0xAD) &	/* A=C   A  */
	(fetch_unibbles(loc+9,4)==0xC80C))	/* PC=(A)   */
      {
	   offs=fetch_unibbles(loc+2,5);
	   sprintf(nbuf, "EXIT_%s", SYMB_RPL(offs));
	   jumpify(loc, nbuf, "%s");
	   return;
      }

     if((fetch_unibbles(loc,2)==0x8F) &		/* GOSBVL x */
	(is_rplloop(loc+7)))			/* LOOP     */
      {
	   sprintf(nbuf,"%s+Lp",SYMB_ABS(fetch_unibbles(loc+2,5)));
	   jumpify(loc, nbuf, "%s");
	   return;
      }
}


/* Decode 80x instruction */
static void decode_group_80()
{
     static char *i807[] = {
	   "OUT=CS", "OUT=C", "A=IN", "C=IN",
	   "UNCNFG", "CONFIG", "C=ID", "SHUTDN" };
     
     int op3, op4, op5, op6;
     
     op3 = get_unibbles(1);
     if(op3 < 8)
      {
	   OUT1(i807[op3]);
	   return;
      }
     
     switch(op3)
      {
      case 9:   OUT1("C+P+1"); break;
      case 0xa: OUT1("RESET"); break;
      case 0xb: OUT1("BUSCC"); break;
      case 0xc: OUT2("C=P\t%d", get_unibbles(1)); break;
      case 0xd: OUT2("P=C\t%d", get_unibbles(1)); break;
      case 0xe: OUT1("SREQ?"); break;
      case 0xf: OUT2("CPEX\t%d", get_unibbles(1)); break;
	   
      case 8:
	   
	   op4 = get_unibbles(1);
	   switch(op4)
	    {
	    case 0: OUT1("INTON"); break;
	    case 3: OUT1("BUSCB"); break;
	    case 4: OUT2("ABIT=0\t%d",get_unibbles(1)); break;
	    case 5: OUT2("ABIT=1\t%d",get_unibbles(1)); break;
	    case 8: OUT2("CBIT=0\t%d",get_unibbles(1)); break;
	    case 9: OUT2("CBIT=1\t%d",get_unibbles(1)); break;
	    case 0xc: OUT1("PC=(A)"); break;
	    case 0xd: OUT1("BUSCD"); break;
	    case 0xe: OUT1("PC=(C)"); break;
	    case 0xf: OUT1("INTOFF"); break;
	    case 1:
		 OUT2("RSI%s", (get_unibbles(1) ? "*" : ""));
		 break;
	    case 2:
		 op5 = get_unibbles(1);
		 if(op5>4)
		      OUT2("LAHEX\t%s",make_hexstr_rev(op5+1));
		 else
		  {
		       op6 = get_unibbles(op5+1);
		       
		       if(op5==4)
			    if(last_cpc == org_pc-4)
			     {
				  add_local_symbol((org_pc+op6)&0xFFFFF,T_SYM_NOHP);
				  OUT2("LA(5)\t(%s)-(*)",SYMB_DAT((org_pc+op6)&0xFFFFF));
			     }
			    else
				 OUT2("LA(5)\t%s", CKSYMB_DAT(op6));
		       else
			    OUT3("LA(%d)\t#%X",op5+1, op6);
		  }
		 if (op5 & 1)
		  {
		       set_pc(pc-op5-1);
		       if(ascii_depth((op5+1)/2) >= ASCLIMIT)
			    force_comment("LASTR '%s'",make_asc((op5+1)/2,HPCHR,YESSPC));
		       else
			    set_pc(pc+op5+1);
		  }
		 break;
	    case 6:
	    case 7:
	    case 0xa:
	    case 0xb:
		 op5 = get_unibbles(1);
		 branch=TRUE;
		 OUT4("?%cBIT=%d\t%d",
			 (op4<8) ? 'A' : 'C',
			 ( (op4==6) || (op4==0xa)) ? 0 : 1,
			 op5);
		 break;
	    }
      }
}



	  
/* Decode instructions starting with 8-f */
static void decode_8_thru_f(op1)
   int op1;
{
     int op, op2, op3, op4, op5, op6;
     static char *fixed_81[]  = {
	   "ASLC", "BSLC", "CSLC", "DSLC",
	   "ASRC", "BSRC", "CSRC", "DSRC",
	   "", "", "", "",
	   "ASRB", "BSRB", "CSRB", "DSRB" },
     
     *fixed_81b[] = {
	   "!81B0?", "!81B1?", "PC=A", "PC=C",
	   "A=PC", "C=PC", "APCEX", "CPCEX",
	   "!81B8?", "!81B9?", "!81BA?", "!81BB?",
	   "!81BC?", "!81BD?", "!81BE?", "!81BF?" },
     
     *fixed_8a9[] = {
	   "?A=B", "?B=C", "?A=C", "?C=D",
	   "?A#B", "?B#C", "?A#C", "?C#D",
	   "?A=0", "?B=0", "?C=0", "?D=0",
	   "?A#0", "?B#0", "?C#0", "?D#0" },

     *fixed_8b9[] = {
	   "?A>B", "?B>C", "?C>A", "?D>C",
	   "?A<B", "?B<C", "?C<A", "?D<C",
	   "?A>=B", "?B>=C", "?C>=A", "?D>=C",
	   "?A<=B", "?B<=C", "?C<=A", "?D<=C" },

     *fixed_c[] = {
	   "A=A+B", "B=B+C", "C=C+A", "D=D+C",
	   "A=A+A", "B=B+B", "C=C+C", "D=D+D",
	   "B=B+A", "C=C+B", "A=A+C", "C=C+D",
	   "A=A-1", "B=B-1", "C=C-1", "D=D-1" },

     *fixed_d[] = {
	   "A=0", "B=0", "C=0", "D=0",
	   "A=B", "B=C", "C=A", "D=C",
	   "B=A", "C=B", "A=C", "C=D",
	   "ABEX", "BCEX", "ACEX", "CDEX" },

     *fixed_e[] = {
	   "A=A-B", "B=B-C", "C=C-A", "D=D-C",
	   "A=A+1", "B=B+1", "C=C+1", "D=D+1",
	   "B=B-A", "C=C-B", "A=A-C", "C=C-D",
	   "A=B-A", "B=C-B", "C=A-C", "D=C-D" },

     *fixed_f[] = {
	   "ASL", "BSL", "CSL", "DSL",
	   "ASR", "BSR", "CSR", "DSR",
	   "A=-A", "B=-B", "C=-C", "D=-D",
	   "A=-A-1", "B=-B-1", "C=-C-1", "D=-D-1" };
     
     op2 = get_unibbles(1);

     switch(op1) {
      case 8:
	   switch(op2)
	    {
	    case 0: decode_group_80(); return;
	    case 1:
		 op3 = get_unibbles(1);
		 if( (op3<=7) || (op3>=0xC))
		  {
		       OUT1(fixed_81[op3]);
		       return;
		  }
		 op4 = get_unibbles(1);
		 switch(op3)
		  {
		  case 8:
		       op5 = get_unibbles(1);
		       op6 = get_unibbles(1);
		       OUT6("%c=%c%cCON\t%s,%d",
			       ('A'+(op5&3)),
			       ('A'+(op5&3)),
			       (op5<8) ? '+' : '-',
			       std_fields[op4],
			       op6+1);
		       return;
		  case 9:
		       op5 = get_unibbles(1);
		       OUT3("%cSRB.F\t%s",
			       ('A'+(op5&3)),
			       std_fields[op4]);
		       return;
		  case 0xa:
		       op5 = get_unibbles(1);
		       op6 = get_unibbles(1);
		       switch(op5)
			{
			case 0: OUT4("R%s=%s.F\t%s", fixed_rf1[op6], fixed_f1[op6], std_fields[op4]);
			     return;
			case 1: OUT4("%s=R%s.F\t%s", fixed_f1[op6], fixed_rf1[op6], std_fields[op4]);
			     return;
			case 2: OUT4("%sR%sEX.F\t%s", fixed_f1[op6], fixed_rf1[op6], std_fields[op4]);
			     return;
			default: OUT2("???.F\t%s",std_fields[op4&7]);
			     return;
			}
		  case 0xb:
		       OUT1(fixed_81b[op4]);
		       switch(op4)
			{
			case 0x4: last_apc = org_pc; break;
			case 0x5: last_cpc = org_pc; break;
			default: break;
			}
		       return;
		  }
		 return;
	    case 2:
		 op3=get_unibbles(1);
		 switch(op3)
		  {
		  case 0x1: OUT1("XM=0"); return;
		  case 0x2: OUT1("SB=0"); return;
		  case 0x4: OUT1("SR=0"); return;
		  case 0x8: OUT1("MP=0"); return;
		  case 0xF: OUT1("CLRHST"); return;
		  default:  OUT2("HS=0\t%d", op3); return;
		  }
	    case 3:
		 op3 = get_unibbles(1);
		 branch=TRUE;
		 switch(op3)
		  {
		  case 1: OUT1("?XM=0"); return;
		  case 2: OUT1("?SB=0"); return;
		  case 4: OUT1("?SR=0"); return;
		  case 8: OUT1("?MP=0"); return;
		  default: OUT2("?HS=0\t%d", op3); return;
		  }
	    case 4:
	    case 5:
		 op3 = get_unibbles(1);
		 OUT3("ST=%d\t%d", (op2 == 4) ? 0 : 1 , op3);
		 return;
	    case 6:
	    case 7:
		 op3 = get_unibbles(1);
		 branch=TRUE;
		 OUT3("?ST=%d\t%d",(op2==6) ? 0 : 1, op3);
		 return;
	    case 8:
	    case 9:
		 op3 = get_unibbles(1);
		 branch=TRUE;
		 OUT3("?P%c\t%d",(op2==8) ? '#' : '=', op3);
		 return;
	    case 0xa:
		 op3 = get_unibbles(1);
		 branch=TRUE;
		 OUT2("%s\tA", fixed_8a9[op3]);
		 return;
	    case 0xb:
		 op3 = get_unibbles(1);
		 branch=TRUE;
		 OUT2("%s\tA", fixed_8b9[op3]);
		 return;
	    case 0xc:
		 op3 = get_nibbles(4);
		 OUT2("GOLONG\t%s", SYMB_JMP((org_pc+2+op3)&0xfffff));
		 comment_jump(org_pc+2+op3);
		 FORMAT(org_pc+2+op3,T_FMT_CODE);
		 return;
	    case 0xd:
		 op3 = get_unibbles(5);
		 OUT2("GOVLNG\t%s", SYMB_JMP(op3));
		 comment_jump(op3);
		 FORMAT(op3,T_FMT_CODE);
		 return;
	    case 0xe:
		 op3 = get_nibbles(4);
		 OUT2("GOSUBL\t%s", SYMB_JMP((org_pc+6+op3)&0xfffff));
		 gosub=TRUE;
		 comment_jump(org_pc+6+op3);
		 FORMAT(org_pc+6+op3,T_FMT_CODE);
		 return;
	    case 0xf:
		 op3 = get_unibbles(5);
		 OUT2("GOSBVL\t%s", SYMB_JMP(op3));
		 gosub=TRUE;
		 comment_jump(op3);
		 FORMAT(op3,T_FMT_CODE);
		 return;
	    }
	   return;
      case 9:
	   op3 = get_unibbles(1);
	   branch=TRUE;
	   if(op2<8)
		OUT3("%s\t%s", fixed_8a9[op3], std_fields[op2]);
	   else
		OUT3("%s\t%s", fixed_8b9[op3], std_fields[op2&7]);
	   return;
      case 0xa:
	   op3 = get_unibbles(1);
	   if(op2<8)
		OUT3("%s\t%s", fixed_c[op3], std_fields[op2]);
	   else
		OUT3("%s\t%s", fixed_d[op3], std_fields[op2&7]);
	   return;
      case 0xb:
	   op3 = get_unibbles(1);
	   if(op2<8)
		OUT3("%s\t%s", fixed_e[op3], std_fields[op2]);
	   else
		OUT3("%s\t%s", fixed_f[op3], std_fields[op2&7]);
	   return;
      case 0xc: OUT2("%s\tA", fixed_c[op2]); return;
      case 0xd: OUT2("%s\tA", fixed_d[op2]); return;
      case 0xe: OUT2("%s\tA", fixed_e[op2]); return;
      case 0xf: OUT2("%s\tA", fixed_f[op2]); return;
      }
}

      
/* Decode instructions starting with '1' */
static void decode_group_1()
{
     int op2, op3, op4;
     
     static char *cmd_13[] = {
	   "D0=A", "D1=A", "AD0EX", "AD1EX",
	   "D0=C", "D1=C", "CD0EX", "CD1EX",
	   "D0=AS", "D1=AS", "AD0XS", "AD1XS",
	   "D0=CS", "D1=CS", "CD0XS", "CD1XS" },

     *cmd_1415[] = {
	   "DAT0=A", "DAT1=A", "A=DAT0", "A=DAT1",
	   "DAT0=C", "DAT1=C", "C=DAT0", "C=DAT1" };
     
     op2 = get_unibbles(1);
     if(op2 < 8)
	  op3 = get_unibbles(1);
     switch(op2)
      {
      case 0: OUT3("R%s=%s", fixed_rf1[op3], fixed_f1[op3]); break;
      case 1: OUT3("%s=R%s", fixed_f1[op3], fixed_rf1[op3]); break;
      case 2: OUT3("%sR%sEX", fixed_f1[op3], fixed_rf1[op3]); break;
      case 3: OUT1(cmd_13[op3]); break;
      case 4: OUT3("%s\t%c", cmd_1415[op3&7], (op3<8) ? 'A' : 'B' ); break;
      case 5:
	   op4 = get_unibbles(1);
	   if(op3 >= 8)
		OUT3("%s\t%d", cmd_1415[op3&7], op4+1);
	   else
		OUT3("%s\t%s", cmd_1415[op3], std_fields[op4]);
	   break;
	   
      case 6: OUT2("D0=D0+\t%d", op3+1); break;
      case 7: OUT2("D1=D1+\t%d", op3+1); break;
      case 8: OUT2("D0=D0-\t%d", get_unibbles(1)+1); break;
      case 9: 
	   op4 = get_unibbles(2);	/* Just data */
	   if(opt_comref)
		OUT2("D0=(2)\t%s", SYMB_DAT( (op4<=0x38) ? (0x100+op4) : (ram_base+op4)));
	   else
		OUT2("D0=(2)\t%s",addrtohex(op4));
	   break;
      case 0xa:
	   OUT2("D0=(4)\t%s",
		(opt_comref) ? SYMB_DAT(ram_base+get_unibbles(4)) :
		addrtohex(get_unibbles(4)));
	   break;
      case 0xb:
	   OUT2("D0=(5)\t%s", CKSYMB_DAT(get_unibbles(5)));
	   break;
      case 0xc: OUT2("D1=D1-\t%d", get_unibbles(1)+1); break;
      case 0xd: 
	   op4=get_unibbles(2);		/* Just data */
	   if(opt_comref)
		OUT2("D1=(2)\t%s", SYMB_DAT( (op3<=0x38) ? (0x100+op4) : (ram_base+op4)));
	   else
		OUT2("D1=(2)\t%s",addrtohex(op4));
	   break;
      case 0xe:
	   if(opt_comref)
		OUT2("D1=(4)\t%s", SYMB_DAT(ram_base+get_unibbles(4)));
	   else
		OUT2("D1=(4)\t%s",addrtohex(get_unibbles(4)));
	   break;
	   
      case 0xf:
	   OUT2("D1=(5)\t%s", CKSYMB_DAT(get_unibbles(5)));
	   break;
      }
}


/* Decode one instruction */
void decode_instr()
{
     int op0, op1, op2, op3;
     
     static char
	  *fixed_0[] = 
	   {
		"RTNSXM", "RTN", "RTNSC", "RTNCC",
		"SETHEX", "SETDEC", "RSTK=C", "C=RSTK",
		"CLRST", "C=ST", "ST=C", "CSTEX",
		"P=P+1", "P=P-1", "***", "RTI"
		};
     
     jumpaddr=0;
     gosub=FALSE;
     if(branch)
      {
	   branch=FALSE;
	   op0=get_nibbles(2);
	   OUT2((op0 ? "GOYES\t%s" : "RTNYES"),
		(op0 ? (char *) SYMB_JMP((org_pc+op0)&0xfffff) :
		 "*"));
	   if(op0)
		comment_jump((org_pc+op0)&0xfffff);
	   FORMAT(org_pc+op0,T_FMT_CODE);
	   return;
      }
     
     op0=get_unibbles(1);
     switch(op0)
      {
      case 0:
	   op1 = get_unibbles(1);
	   if(op1 != 0xe)
	    {
		 OUT1(fixed_0[op1]); return;
	    }
	   op2 = get_unibbles(1);
	   op3 = get_unibbles(1);
	   OUT3("%s\t%s", fixed_0e[op3], std_fields[op2 & 017]);
	   return;
	   
      case 1:
	   decode_group_1();
	   return;
	   
      case 2:
	   op2 = get_unibbles(1);
	   OUT2("P=\t%d", op2);
	   return;
	   
      case 3:
	   op2 = get_unibbles(1);
	   if(op2>4)
		OUT2("LCHEX\t%s",make_hexstr_rev(op2+1));
	   else
	    {
		 op3 = get_unibbles(op2 + 1);
		 if(op2==4)
		      if(last_apc == org_pc-4)
		       {
			    add_local_symbol((org_pc+op3)&0xFFFFF,T_SYM_NOHP);
			    OUT2("LC(5)\t(%s)-(*)", SYMB_DAT((org_pc+op3)&0xFFFFF));
		       }
		      else
			   OUT2("LC(5)\t%s", CKSYMB_DAT(op3));
		 else
		      OUT3("LC(%d)\t#%X", op2+1, op3);
	    }
	   if (op2 & 1)
	    {
		 set_pc(pc-op2-1);
		 if(ascii_depth((op2+1)/2) >= ASCLIMIT)
		      force_comment("LCSTR '%s'",make_asc((op2+1)/2,HPCHR,YESSPC));
		 else
		      set_pc(pc+op2+1);
	    }
	   return;
	   
      case 4:
	   op2 = get_nibbles(2);
	   if(op2 == 0x02)
		OUT1("NOP3");
	   else
		if(op2)
		 {
		      OUT2("GOC\t%s", SYMB_JMP((org_pc+1+op2) & 0xfffff));
		      comment_jump(org_pc+1+op2);
		      FORMAT(org_pc+1+op2,T_FMT_CODE);
		 }
		else
		     OUT1("RTNC");
	   return;
	   
      case 5:
	   if(op2 = get_nibbles(2))
	    {
		 OUT2("GONC\t%s", SYMB_JMP((org_pc+1+op2) & 0xfffff));
		 comment_jump(org_pc+1+op2);
		 FORMAT(org_pc+1+op2,T_FMT_CODE);
	    }
	   else
		strcpy(l_instr, "RTNNC");
	   return;
	   
      case 6:
	   op2 = get_nibbles(3);
	   if(op2 == 0x003)
		OUT1("NOP4");
	   else if(op2 == 0x004)
	    {
		 get_unibbles(1);
		 OUT1("NOP5");
	    }
	   else
	    {
		 OUT2("GOTO\t%s", SYMB_JMP((org_pc+1+op2) & 0xfffff));
		 comment_jump((org_pc+1+op2) & 0xfffff);
		 FORMAT(org_pc+1+op2,T_FMT_CODE);
	    }
	   return;
	   
      case 7:
	   op2 = get_nibbles(3);
	   OUT2("GOSUB\t%s",SYMB_JMP((org_pc+4+op2) & 0xfffff));
	   gosub=TRUE;
	   comment_jump(org_pc+4+op2);
	   FORMAT(org_pc+4+op2,T_FMT_CODE);
	   return;
	   
      default:
	   decode_8_thru_f(op0);
	   return;
      }
}
