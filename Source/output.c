#include <stdio.h>
#include <signal.h>
#include "globals.h"


/* ********************************************************************** */
/* *               Error handlers                                       * */
/* ********************************************************************** */


/* For debugging purposes */
void debug_msg(ctrl, a1, a2, a3, a4, a5, a6, a7, a8)
    char *ctrl, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
{
     if(!opt_debug)
	  return;
     fprintf(stderr, ctrl, a1, a2, a3, a4, a5, a6, a7, a8);
}


/* Issue an error */
void error(ctrl, a1, a2, a3, a4)
    char *ctrl, *a1, *a2, *a3, *a4;
{
     if(opt_lines)
	  fprintf(stderr, "%d: ", lineno);
     
     fputs("ERROR - ", stderr);
     fprintf(stderr, ctrl, a1, a2, a3, a4);
     fputc('\n', stderr);
}


/* Issue a warning */
void warning(ctrl, a1, a2, a3, a4)
    char *ctrl, *a1, *a2, *a3, *a4;
{
     if(opt_lines)
	  fprintf(stderr, "%d: ", lineno);
     fputs("WARNING - ", stderr);
     fprintf(stderr, ctrl, a1, a2, a3, a4);
     fputc('\n', stderr);
}


/* Dump sad error data and exit */
void saderror()
{
     error("Location   : %05X-%05X", org_pc, pc);
     error("Pass       : %d", pass);
     error("Source     : %s", l_source);
     error("Code       : %s", l_code);
     error("Instruction: %s", l_instr);
     error("Comments   : %s", l_comment);
     exit(1);
}


void sighandle(sig)
    int sig;
{
     switch(sig) {
      case SIGSEGV:
	   error("Internal SAD error (SIGSEGV).");
	   break;
      case SIGBUS:
	   error("Internal SAD error (SIGBUS).");
	   break;
      default:
	   error("Internal SAD error (Unknown).");
	   break;
      }
     saderror();
}


/* Disable signals interrupt and quit signals */
void disable_intr()
{
     signal(SIGHUP, SIG_IGN);
     signal(SIGINT, SIG_IGN);
     signal(SIGQUIT, SIG_IGN);
     signal(SIGTERM, SIG_IGN);
     signal(SIGPIPE, SIG_IGN);
     signal(SIGUSR1, SIG_IGN);
     signal(SIGUSR2, SIG_IGN);
     
#ifdef SIGSYS
     signal(SIGSYS, SIG_IGN);
#endif
     
#ifdef SIGSTOP
  signal(SIGSTOP, SIG_IGN);
#endif
     
#ifdef SIGTSTP
     signal(SIGTSTP, SIG_IGN);
#endif
     
#ifdef SIGWINCH
     signal(SIGWINCH, SIG_IGN);
#endif
}


/* ********************************************************************** */
/* *               Output of data tables                                * */
/* ********************************************************************** */

/* Fetch address for name from the other symbol table */
/* Very slow, gotta improve it */

static int get_addr_other(name)
    char *name;
{
     int cur_addr = -1;
     FILE *symfile;
     char lbuf[132], *cp;

     if(!opt_gx)
	  symfile = fopen(SAD_SYMBOLSGX, "r");
     else
	  symfile = fopen(SAD_SYMBOLS, "r");
    
     if(!symfile)
	  return(-1);
     
     /* Search xxxxx[=.;]name */
     for(; !feof(symfile); )
      {
	   fgets(lbuf, sizeof lbuf, symfile);
	   if(feof(symfile))
		break;
	   lbuf[strlen(lbuf)-1] = '\0';
	   cp = lbuf;
	   hexstrtoi(&cp, &cur_addr);
	   if(*cp++ != '\0')
		if(strcmp(name,cp)==0)
		 {
		      fclose(symfile);
		      return(cur_addr);
		 }
      }
     fclose(symfile);
     return(-1);
}


/* Print symbol definitions */
void print_sym_defs()
{
     int i;
     int addr2;
     register struct symbol *symp;

     if(opt_hp & !opt_symdef_all)
	  return;

     putchar('\n');
     separate(TRUE);
     if(opt_symdef_all)
	  printf("*\tUsed entries\n");
     else
	  printf("*\tUnsupported entries\n");
     separate(TRUE);
     puts("ASSEMBLE");


     /* Use sorted symref array for printing */
     for(i=0 ; symp = symref[i]; i++)

/*     for(symp = symroot; symp; symp = symp->link) */

      {
	   if((symp->ref) && !(symp->type & T_SYM_ISDEF))
		if(opt_symdef_all | ((symp->type & T_SYM_TYPE) != T_SYM_HP))
		 {
		      if (symp->type & T_SYM_LOCAL)
			   printf("%s\t%sEQU #%05X",
				  local_id(symp),
				  strlen(local_id(symp))<8 ? "\t" : "",
				  symp->val);
		      else if(symp->val > 0xFFFFF)
			   printf("DEFINE %s\tROMPTR %X %X",
				  symp->id,
				  (symp->val >> 12) & 0xFFF,
				  symp->val & 0xFFF);
		      else
			   printf("=%s\t%sEQU #%05X",
				  symp->id,
				  strlen(symp->id)<7 ? "\t" : "",
				  symp->val);
		      if(symp->val <= 0xFFFFF)
			   if ((symp->type & T_SYM_TYPE) == T_SYM_REVDEP)
			    {
				 printf("\t** %s. %s: ",
					STR_VERSDEPND,
					opt_gx ? "SX" : "GX");
				 addr2 = get_addr_other(symp->id);
				 if(addr2 != -1)
				      printf("#%05X **", addr2);
				 else
				      printf("Unknown **");
			    }
			   else if(!opt_source)
				if ((symp->type & T_SYM_TYPE) != T_SYM_HP)
				     printf("\t** %s **", STR_UNSUPPORTED);
		      putchar('\n');
		 }
      }
     puts("RPL");
}


/* Print XREF data */
void print_xref()
{
     int i;
     register struct symbol *symp;
     register struct xrefaddr *xrefp;
     
     putchar('\n');
     putchar('\n');
     separate(TRUE);
     printf("*\tCross References\n");
     separate(TRUE);
     
     /* Use sorted symref array for printing */
     for(i=0 ; symp = symref[i]; i++)

/*     for(symp = symroot; symp; symp = symp->link) */
	  if(symp->ref)
	   {
		int tabpos=0;
		char lbuf[132];
		
		if(symp->type & T_SYM_LOCAL)
		      printf("\n* %s", local_id(symp));
		else
		     if(symp->val <= 0xFFFFF)
			  printf("\n* %s", symp->id);
		     else
			  printf("\n* ROMPTR %X %X ( %s )",
				 (symp->val >> 12) & 0xFFF,
				 symp->val & 0xFFF,
				 symp->id);
		fputs("\n*\t:", stdout);
		for(xrefp = symp->xrefhead; xrefp; xrefp = xrefp->link)
		 {
		      int w;
		      sprintf(lbuf, "\t%s", addrtohex(xrefp->val));
		      sprintf(lbuf, "\t%s", addrtohex(xrefp->val));
		      tabpos+=8;
		      if(tabpos > 59)
		       {
			    fputs("\n*\t:", stdout);
			    tabpos = 0;
		       }
		      fputs(lbuf, stdout);
		 }
	   }
     putchar('\n');
     separate(TRUE);
}


/* Print comment file on stdout */
void print_comfile(comfname)
    char *comfname;
{
     int i;
     struct symbol *comp;
     FILE *comfile;
     
     comfile = stdout;
     
     if(comfname)
	  disable_intr();		/* No interruptions */
     
     if(comfname && !(comfile = fopen(comfname, "w")))
      {
	   perror(comfname);
	   exit(1);
      }

     for(i=0; comp = comref[i]; i++)

/*     for(comp = comroot; comp; comp = comp->link)  */
	  if(!(comp->type & T_COM_ERASE))
	   {
		fprintf(comfile, "%05X", comp->val);
		
		switch(comp->type & T_COM_TYPE)
		 {
		 case T_COM_MAJOR:
		      fputc('=', comfile);
		      break;
		 default:
		      fputc(':', comfile);
		      break;
		 }
		fputs(comp->id, comfile);
		fputc('\n', comfile);
	   }
}


/* Print symbol file on stdout */
void print_symfile(symfname)
    char *symfname;
{
     struct symbol *symp;
     FILE *symfile;
     int i;

     symfile = stdout;
     
     if(symfile)
	  disable_intr();		/* Disable various signals */

     if(symfname && !(symfile = fopen(symfname, "w")))
      {
	   perror(symfname);
	   exit(1);
      }
     
     for(i=0; symp = symref[i]; i++)
	  if(!(symp->type & T_SYM_ERASE))
	   {
		fprintf(symfile, "%05X", symp->val);
		
		switch(symp->type & T_SYM_TYPE)
		 {
		 case T_SYM_HP:
		      fputc('=', symfile);
		      break;
		      
		 case T_SYM_NOHP:
		      fputc(':', symfile);
		      break;
		 case T_SYM_REVDEP:
		 default:
		      fputc(',', symfile);
		      break;
		 }
		
		fputs(symp->id, symfile);
		fputc('\n', symfile);
	   }
}


/* ********************************************************************** */
/* *               Disassembly output                                   * */
/* ********************************************************************** */


/* Print separation line */
void separate(force)
    int force;
{
     if( force | ((pass==PASS2) & opt_code) )
	  puts("*********************************************************************");
}


/* Skip whitespace */
char *byspace(str)
    register char *str;
{
     while(*str && *str <= '\040')
	  str++;
     return(str);
}


/* Return position where cursor would be, were we to
 * print this string following CR.
 */
static prpos(str)
       register char *str;
{
     register pos;
     
     for(pos = 0; *str;)
	  switch(*str++)
	   {
	   case '\t':
		pos = (pos & ~7) + 8;
		break;
	   case '\n':
		pos = 0;
		break;
	   default:
		pos++;
	   }
     return(pos);
}


/* Tabulate to specific column.
 * Note: column must be an even multiple of 8.
 */
static void tabulate(from, to)
     register  from, to;
{
     while(from < to)
      {
	   putchar('\t');
	   from = (from & ~7) + 8;
      }
}


/* Tell from instruction buffer whether disassembly failed */
static int failed(str)
    register char *str;
{
     char delim;
     
     if(!str || !*str)
	  return(TRUE);
     
     for(; *str; str++)
	  if(*str == '\\' && str[1])
	       str++;
	  else
	       if(*str == '"' || *str == '\'')
		    for(delim = *str++; *str && *str != delim; str++)
			 if(*str == '\\' && str[1])
			      str++;
     return(FALSE);
}


/* Edit out (sigh) spaces following colons. */
static void edit_out_spaces_following_commas(str)
       register char *str;
{
     register char delim, *dst;
     
     if(!str || !*str)
	  return;
     
     for(dst = str; *str; *dst++ = *str++)
	  if(*str == '\\' && str[1])
	       *dst++ = *str++;
	  else
	       if(*str == '"' || *str == '\'')
		    for(delim = *dst++ = *str++;
			*str && *str != delim; 
			*dst++ = *str++)

			 if(*str == '\\' && str[1])
			      *dst++ = *str++;
			 else ;
	       else
		    if(*str == ',')
		     {
			  *dst++ = *str++;
			  str = byspace(str);
		     }
		    else
			 if(*str == ';')
			  {
			       while(*dst++ = *str++);
			       return;
			  }
     *dst = '\0';
}


/* Return spaces according to current indentation level */
static char *rpl_indent()
{
     static char spaces[256];
     
#ifndef SYSV
     register char *cp;
#endif
     
     int ind;

     /* Make indentation checks */
     if(rpl_comp_level==0)
      {
	   rpl_ind_level = rpl_new_ind_level = 0;
      }
     else if (rpl_ind_level < rpl_comp_level)
	  if (rpl_new_ind_level < rpl_comp_level)
	   {
	   rpl_ind_level = rpl_comp_level;
	   rpl_new_ind_level = rpl_ind_level;
      }

     ind = rpl_ind_level * 2;
     /* Has to be here if ind. is changed outside main rpl loop */
     rpl_ind_level = rpl_new_ind_level;

#ifdef SYSV
     memset(spaces, ' ', ind);
     spaces[ind] = '\0';
#else
     for(cp = spaces; cp < spaces + ind; *cp++ = ' ');
     *cp = '\0';
#endif
     return(spaces);
}

static print_source_no_newline()
{
     if(pass==PASS1)
	  return;

     if(rpl_comp_level < dispatch_level)
	  mode_dispatch = FALSE;
     if(mode_dispatch)
      {
	   if(rpl_comp_level == dispatch_level)
	    {
		 dispatch_lines--;
		 mode_fixed_line = TRUE;
		 fixed_ops = dispatch_obs;
	    }
	   else
		mode_fixed_line = FALSE;
	   if(dispatch_lines < 0)
		mode_dispatch = FALSE;
      }
     else
      {
	   mode_fixed_line = FALSE;
	   fixed_ops = 0;
      }
     /* Even blank lines count in above calculations!!! */
     if(source_ind==0)
	  return;
     fputs(l_source, stdout);
     l_sourcep = l_source;
     source_ind = 0;
}

void print_source()
{
     if(pass==PASS1)
	  return;
     if(source_ind==0)
	  print_source_no_newline();
     else
      {
	   print_source_no_newline();
	   putchar('\n');
      }
}


/* Print disassembled line */
void print_line()
{
     struct symbol tmpsym, *local_symp, *lname_symp;

     char
	  *lcp = l_code,
	  code[1024], lbuf[1024],
	  pc_str[32], pc_str_blank[32],
	  *indstr;
     int lpos, max_codelen;
     int local_exists, global_exists, lname_exists;
     int temp;

     /* No printing unless pass 2 and code enabled */
     if(pass != PASS2 || !opt_code)
      {
	   /* Attempted to pass a format point? If so, adjust the PC */
	   if(nextfmt && (*nextfmt) && (*nextfmt)->val < pc && 
	      (*nextfmt)->val > org_pc)
	    {
		 set_pc((*nextfmt)->val);
		 prevfmt = NULL;
		 fmtexpire_tag = 0xffffff;
		 /*	  rpl_new_ind_level--;
			  rpl_ind_level--; */
	    }
	   l_commentp = l_comment;
	   return;
      }
     
     max_codelen = (opt_alonzo ? 12 : 7);
     indstr = rpl_indent();
     
     *l_codep = '\0';
     
     if(opt_alonzo)
      {
	   sprintf(pc_str, "#%05X:", org_pc);
	   sprintf(pc_str_blank, "#%05X:   ", org_pc);
	   edit_out_spaces_following_commas(l_instr);
#ifdef ALONZO_PATCH
	   assemble(l_instr);
#endif
      }
     else
      {
	   sprintf(pc_str, "%05X", org_pc);
	   sprintf(pc_str_blank, "%05X   ", org_pc);
      }
     
     /* Attempted to pass a format point? If so,
      * we need to adjust the PC, by outputting a dummy data op.
      * and wind back the PC. Next time around, we'll get the real
      * thing.
      */
     if(nextfmt && (*nextfmt) && (*nextfmt)->val < pc && 
	(*nextfmt)->val > org_pc)
      {
	   int nibbles = (*nextfmt)->val - org_pc, n;
	   register char *cp, *bp;
	   
	   set_pc((*nextfmt)->val);
	   prevfmt = NULL;
	   fmtexpire_tag = 0xffffff;
	   /*      rpl_new_ind_level--;
		   rpl_ind_level--; */
	   
	   /* Create data statement */
	   new_mode_rpl=FALSE;
	   OUT2("CON(%d)\t",nibbles);
	   bp = l_instr + strlen(l_instr);
	   cp = l_code + nibbles;
	   *cp-- = '\0';
	   /* Skip leading zeroes */
	   while(cp > l_code && *cp == '0')
		cp--;
	   /* Make it hex, if necessary */
	   if(!(cp == l_code && *cp <= '9'))
		*bp++ = '#';
	   /* Duplicate */
	   while(cp >= l_code)
		*bp++ = *cp--;
	   *bp = '\0';
	   /* Add comment */
	   force_comment("Sync");
      }
     
     code[max_codelen] = '\0';
     strncpy(code, l_code, max_codelen);
     if(strlen(l_code) > max_codelen)
	  lcp += max_codelen;
     else
	  lcp += strlen(code);
     
     /* Major comment point? */
     if(!mode_endcode && nextcom && *nextcom && (*nextcom)->val == org_pc &&
	(*nextcom)->type == T_COM_MAJOR)
      {
	   separate(FALSE);
	   
	   while(*nextcom && (*nextcom)->val == org_pc &&
		 (*nextcom)->type == T_COM_MAJOR)
	    {
		 printf("* %s\n", (*nextcom)->id);
		 nextcom++;
	    }
      }
     
     /* Symbol point? */

     if(!mode_endcode && nextsym && *nextsym && (*nextsym)->val == org_pc)
      {
	   local_exists = global_exists = lname_exists = FALSE;
	   lname_symp = NULL; local_symp = NULL;
	   
	   /* Scan formats for global labels */

	   while(nextsym && *nextsym && (*nextsym)->val == org_pc)
	    {
	      int tag;
		
	      /* Print label */
	      
	      if( (*nextsym)->type & T_SYM_LOCAL )
	       {
		    local_symp = *nextsym;
		    local_exists=TRUE;
	       }
	      else if( ((*nextsym)->type & T_SYM_TYPE)==T_SYM_LNAME)
	       {
		    lname_symp = *nextsym;
		    lname_exists=TRUE;
	       }
	      else
	       {
		    print_source();
		    if(!global_exists)
		     {
			  separate(FALSE);
			  global_exists=TRUE;
			  
			  if(mode_rpl & !new_mode_rpl )
			   {
				print_source();
				printf("ASSEMBLE\n");
			   }
			  else if (!mode_rpl & new_mode_rpl )
			       printf("RPL\n");
			  mode_rpl=new_mode_rpl;
		     }
		    
		    /* Print pc string */
		    if(opt_opcode)
			 fputs(pc_str_blank, stdout);
		    
		    if(!opt_source |
		       ((*nextsym)->type & T_SYM_TYPE) != T_SYM_ROMP)
		     {
			  if(mode_rpl)
			       printf("LABEL ");
			  else
			       putchar('=');
			  printf("%s", (*nextsym)->id);
			  if(((*nextsym)->type & T_SYM_TYPE) == T_SYM_ROMP)
			       if(fetch_unibbles(org_pc-6,3) == libnum)
				    printf(" ( xNAME )");
			       else
				    printf(" ( NULLNAME )");
			  else if(((*nextsym)->type & T_SYM_TYPE) == T_SYM_REVDEP)
			       printf("\t( %s )", STR_VERSDEPND);
			  else if(((*nextsym)->type & T_SYM_TYPE) != T_SYM_HP)
			       printf("\t( %s )", STR_UNSUPPORTED);
			  putchar('\n');
		     }
		    else
		     {
			  if(fetch_unibbles(org_pc-6,3) == libnum)
			   {
				/* Stripping leading 'x' */
				static char xname[20], *xnamep;
				xnamep = xname;
				sprintf(xname, "%s", (*nextsym)->id);
				
				/* Some people use xNAMEs with no names */
				/* Better check if the name was generated */
				/* by sad itself. */
				if(strncmp(xname, "XLIB_", 5)==0)
				     printf("%sxNAME %s", indstr, xnamep);
				else if (strncmp(xname, "xXLIB_",6)==0)
				 {
				      /* It's hNAME */
				      *xnamep = ' ';
				      printf("%shNAME%s", indstr, xnamep);
				 }
				else
				 {
				      *xnamep = ' ';
				      printf("%sxNAME%s", indstr, xnamep);
				 }
			   }
			  else
			       printf("%sNULLNAME %s",indstr,(*nextsym)->id);
			  putchar('\n');
		     }
	       }
	      
	      (*nextsym)->type |= T_SYM_ISDEF;
	      nextsym++;
	      
	 }
	   
	   if(!(opt_source & global_exists))
	    {
		 if(lname_exists)
		  {
		       if(opt_opcode)
			    fputs(pc_str_blank, stdout);
		       if(mode_rpl)
			    printf("LOCALLABEL %s\n", local_id(lname_symp));
		       else
			    printf("%s\n", local_id(lname_symp));
		  }
		 else if(local_exists)
		  {
		       if(opt_opcode)
			    fputs(pc_str_blank, stdout);
		       if(mode_rpl)
			    printf("LOCALLABEL %s\n", local_id(local_symp));
		       else
			    printf("%s\n", local_id(local_symp));
		  }
	    }     
      }
     /* This segment in in 2 places to make disassembly more beautiful */
     /* by inserting the change to a natural position */
     
     if(mode_rpl & !new_mode_rpl )
      {
	   print_source();
	   printf("ASSEMBLE\n");
      }
     else if (!mode_rpl & new_mode_rpl )
	  printf("RPL\n");
     mode_rpl=new_mode_rpl;
     

     /* Symbol caught in-between? */

     temp = org_pc;		/* Used to prevent duplicate comments */
     while(!mode_endcode &&
	   nextsym && *nextsym &&
	   ((*nextsym)->val > org_pc) &&
	   ((*nextsym)->val < pc))
      {
	   if( (*nextsym)->val != temp )
		comment("%s=#%X", 
			((*nextsym)->type & T_SYM_LOCAL ?
			 local_id(*nextsym) : (*nextsym)->id),
			(*nextsym)->val);
	   temp = (*nextsym)->val;
	   nextsym++;
      }
     
     /* Comment caught in-between?
      * Then skip, regardless of type.
      */
     while(!mode_endcode && nextcom && *nextcom && (*nextcom)->val > org_pc &&
	   (*nextcom)->val < pc)
	  nextcom++;
     
     
     /* Now check if we actually managed to disassemble the instruction.
      * If l_instr is blank, or contains wildcard characters (*'s),
      * we'll substitute it with a suitable data instruction.
      */
     
     /* I changed this in SADHP because many supported commands use '*' */
     
     if(failed(l_instr))
      {
	   int n = strlen(l_code);
	   register char *cp, *bp;
	   
	   new_mode_rpl=FALSE;
	   if(mode_rpl & !new_mode_rpl )
	    {
		 print_source();
		 printf("ASSEMBLE\n");
	    }
	   mode_rpl=new_mode_rpl;
	   
	   if(l_instr[0])
		comment("%s", l_instr);
	   
	   OUT2("CON(%d)\t",n);
	   
	   bp = l_instr + strlen(l_instr);
	   cp = l_code + n - 1;
	   
	   /* Skip leading zeroes */
	   while(cp > l_code && *cp == '0')
		cp--;
	   
	   /* Make it hex, if necessary */
	   if(!(cp == l_code && *cp <= '9'))
		*bp++ = '#';
	   
	   /* Duplicate */
	   while(cp >= l_code)
		*bp++ = *cp--;
	   
	   *bp = '\0';
      }
     
     /* Print instruction */

     if(mode_single_op)
	  print_source();
     
     if(!mode_single_op & opt_source & mode_rpl)
      {
	   int linelen = strlen(l_instr);
	   
	   if(!mode_fixed_line & mode_dispatch & 
	      (rpl_comp_level==dispatch_level))
	    {
		 mode_fixed_line = TRUE;
		 fixed_ops = dispatch_obs;
	    }
	   
	   if (((mode_fixed_line ? (fixed_ops < 0 ) : FALSE) |
	       ((source_ind+linelen > SRC_LINE_MAX) & (source_ind != 0)))
	       & (mode_fixed_line ? (fixed_ops >1 ) : TRUE))
	    {
		 print_source();
		 sprintf(l_sourcep, "%s%s", indstr, l_instr);
		 l_sourcep += strlen(l_sourcep);
		 source_ind = strlen(l_source);
	    }
	   else	if(source_ind == 0)
	    {
		 sprintf(l_sourcep, "%s%s", indstr, l_instr);
		 source_ind = strlen(l_sourcep);
		 l_sourcep += source_ind;
	    }
	   else
	    {
		 *l_sourcep++ = ' ';
		 strcpy(l_sourcep, l_instr);
		 source_ind += strlen(l_sourcep)+1;
		 l_sourcep += strlen(l_sourcep);
	    }
	   if (mode_fixed_line & (--fixed_ops < 0))
		 print_source();

      }
     else
      {
	   if(opt_source & mode_rpl)
		sprintf(lbuf, "%s%s", indstr, l_instr);
	   else if(!opt_opcode)
		sprintf(lbuf, "\t%s", l_instr);
	   else
		sprintf(lbuf,
			(opt_alonzo ? "%s #%-12s  %s%s" : "%s %-7s %s%s"),
			pc_str,
			code,
			(mode_rpl ? indstr : "\t"), 
			l_instr);
	   fputs(lbuf, stdout);
      }
     
     /* Add comments */

     /*     if(!mode_endcode &&
      *	((l_comment[0] && opt_comments) ||
      *	 (nextcom && *nextcom && (*nextcom)->val == org_pc)))
      */

     /* Changed so that force_comment() will always work  */

     if(!mode_endcode &&
   	(l_comment[0] || (nextcom && *nextcom && (*nextcom)->val == org_pc)))
      {
	   if(!mode_single_op & opt_source & mode_rpl)
		lpos = source_ind;
	   else
		lpos = prpos(lbuf);
	   
	   print_source_no_newline();

	   if(lpos >= opt_commentcol)
		putchar(' ');
	   else
		tabulate(lpos, opt_commentcol);
	   
	   if(mode_rpl & !opt_opcode)
		putchar('(');
	   else
		putchar('*');
	   
	   if(nextcom && *nextcom && (*nextcom)->val == org_pc)
	    {
		 printf(" %s", (*nextcom)->id, stdout);
		 nextcom++;
		 if(l_comment[0])
		      printf(" *_ %s", l_comment+1);
	    }
	   else if(l_comment[0])
		printf("%s %s", (opt_opcode ? "_" : ""), l_comment+1);

	   if(mode_rpl & !opt_opcode)
		printf(" )");
	   putchar('\n');
      }
     else if(!mode_rpl | !opt_source | mode_single_op )
	  putchar('\n');
     
     /* List any remaining code and/or comments */
     while(!mode_endcode &&
	   ((opt_opcode && *lcp) ||
	    (nextcom && *nextcom && (*nextcom)->val == org_pc)))
      {
	   int lpos = 0;
	   
	   l_instr[0] = '\0';
	   
	   if(opt_opcode && *lcp)
	    {
		 strncpy(code, lcp, max_codelen);
		 if(strlen(lcp) > max_codelen)
		      lcp += max_codelen;
		 else
		      lcp += strlen(lcp);
		 
		 if(!opt_source)
		      if(opt_alonzo)
			   printf("       #%s", code);
		      else
			   printf("      %s", code);
		 
		 lpos = strlen(code) + 6;
	    }
	   
	   if(nextcom && *nextcom && (*nextcom)->val == org_pc)
	    {
		 tabulate(lpos, opt_commentcol);
		 
		 if(!opt_source)
		      printf("* %s", (*nextcom)->id);
		 nextcom++;
	    }
	   
	   putchar('\n');
      }
}
