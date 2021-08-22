#ifndef _GLOBAL_CONFG_
#define _GLOBAL_CONFG_

static char
     sad_version[] = "HP 1.08";

static char
     spc_chars[] = "=#><+-!&()? 0123456789ABCDEFGHIJKLMNOP RSTUVWXYZ";

#define SPC_CHARS 0x2F

#define STR_VERSDEPND	"Version dependant"
#define STR_UNSUPPORTED	"Unsupported"

static char *macro_prop[] = {
      "soMACRO", "begMACRO", "midMACRO", "endMACRO",
      "hasSST", "hasPROMPT" };

static char *function_prop[] = {
      "hasEQWR", "hasAKA", "hasDISP",
      "hasDER", "hasRINV", "hasCOLCT", "hasEXPND",
      "hasRULES", "hasINTG", "hasWHERE", "hasVUNS" };
  
#define SAD_CORE        ".core"	       /* Core file */
#define SAD_COREGX      ".core.gx"     /* Core file for GX */
#define SAD_CORE49G     ".System"      /* Core file for 49G */
#define SAD_SYMBOLS     ".symbols"     /* Symbols file */
#define SAD_SYMBOLSGX   ".symbols.gx"  /* Symbols file for GX */
#define SAD_COMMENTS49G ".comments"    /* Comments file for 49G */
#define SAD_COMMENTS    ".comments.48" /* Comments file */
#define SAD_COMMENTSGX  ".comments.48" /* Comments file for GX */
#define SAD_FORMATS49G  ".formats"     /* Formats file for 49G */
#define SAD_FORMATS     ".formats.sx"  /* Formats file */
#define SAD_FORMATSGX   ".formats.gx"  /* Formats file for GX */
#define SAD_MACROS      ".macros"      /* Macro pattern file */
#define SAD_AUTOFMT     "formats.out"  /* Formats auto gen output */

#define SAD_CORE1      ".port1"
#define SAD_SYMBOLS1   ".symbols1"
#define SAD_FORMATS1   ".formats1"
#define SAD_COMMENTS1  ".comments1"

#define SAD_CORE2      ".port2"
#define SAD_SYMBOLS2   ".symbols2"
#define SAD_FORMATS2   ".formats2"
#define SAD_COMMENTS2  ".comments2"


#define JUMPREL  0x40		/* What is considered a routine offset */
#define DATAREL  0x6		/* What is considered a var offset */
#define NOREL    0
#define NORELRPL -1             /* Zero offset, rpl form */

#define COMMENT_COL 40
#define SRC_LINE_MAX 55

#define PASS1 0			/* Pass 1 (collection) */
#define PASS2 1			/* Pass 2 (output) */
#define PASSF 2			/* Pass F (format extraction) */

#define NOSPC 0			/* Spaces not allowed in strings */
#define YESSPC 1		/* Spaces allowed in strings */

#define SPCHR 1			/* Special character set */
#define HPCHR 0			/* Normal charset */

#define SCODE 1			/* Name is for code */
#define SRPL  0			/* Name is for rpl */

/* Help macros for output etc */
#define OUT1(a1)		strcpy (l_instr, a1 )
#define OUT2(a1,a2)		sprintf(l_instr, a1,a2 )
#define OUT3(a1,a2,a3)		sprintf(l_instr, a1,a2,a3 )
#define OUT4(a1,a2,a3,a4)	sprintf(l_instr, a1,a2,a3,a4 )
#define OUT5(a1,a2,a3,a4,a5)	sprintf(l_instr, a1,a2,a3,a4,a5 )
#define OUT6(a1,a2,a3,a4,a5,a6)	sprintf(l_instr, a1,a2,a3,a4,a5,a6 )
#define OUTCON0()		strcpy (l_instr, "CON(5)\t0")
#define OUTREL5(a1)		sprintf(l_instr, "REL(5)\t%s", a1)
#define OUTCON5(a1)		sprintf(l_instr, "CON(5)\t%s", a1)
#define OUTNIBHEX(a1)		sprintf(l_instr, "NIBHEX\t%s", make_hexstr(a1))


#define FORMAT(a1,a2) \
     if(opt_formats && pass == PASSF ) add_auto_format(a1,a2)

#define SYMB_ABS(a)	symbolic(a,NOREL,SCODE)
#define SYMB_DAT(a)	symbolic(a,DATAREL,SCODE)
#define SYMB_JMP(a)	symbolic(a,JUMPREL,SCODE)
#define SYMB_RPL(a)	symbolic(a,NORELRPL,SRPL)
#define CKSYMB_ABS(a)	cksymbolic(a,NOREL,SCODE)
#define CKSYMB_DAT(a)	cksymbolic(a,DATAREL,SCODE)
#define CKSYMB_JMP(a)	cksymbolic(a,JUMPREL,SCODE)
#define CKSYMB_RPL(a)	cksymbolic(a,NORELRPL,SRPL)

/* True if char is likely to appear in strings */
#define IS_GOOD_CHAR(C) \
     ( ((C >= ' ') & (C < 127)) | (C == '\n') | (C == '\t'))

#define ASCLIMIT 75		/* Required percentage */

/* True if char is good for initial pos in sym id */
#define IS_INITIAL_SYM(C) \
  (((C) >= 'a' && (C) <= 'z') || \
   ((C) >= 'A' && (C) <= 'Z') || \
   (C)== '_' || \
   (C) < 0)                     /* ISO 8859-1, upper half */

/* True if char is good for any pos within sym id */
#define IS_INNER_SYM(C) \
  ((C) > ' ' && (C) != ':' && (C) != '=' && (C) != ';' && \
   (C) != '/' && (C) != '#' && (C) != '.' && (C) != '<' && \
   (C) != '>' && (C) != ',' && (C) != '(' && (C) != ')')


/* Xref node */
struct xrefaddr {
  int val;			/* XREF value */
  struct xrefaddr *link;	/* Next in chain */
};


/* This is just so that programs that never will
 * use formats don't have to include format.h
 */
#ifndef _FMT_CONFG_
#define FMT char
#else
#define FMT struct format
#endif


/* Comments are considered to be symbols,
 * and reside in the comment table.
 */
struct symbol {
      int
	   val,			/* Symbol value or macro tag */
	   ref,			/* Reference counter (for Xref) */
	   seq;			/* Sequence counter, or pattern size */

  struct symbol *link;		/* Next symbol */

  union {
    char *str;			/* Symbol ID, comment string, macro def */
    FMT
      *format;			/* Format description */
  }
  s_v;

  unsigned char
    type;			/* T_xxx */

  struct xrefaddr
    *xrefhead,			/* XREF address list */
    *xreftail;
};


#define id s_v.str		/* For symbol/comment reference */
#define form s_v.format		/* For format reference */

#define T_COM_TYPE  0x7		/* Type field mask */
#define T_COM_MINOR 0x0		/* Comment is `minor comment' */
#define T_COM_MAJOR 0x1		/* Comment is `major comment' */
#define T_COM_ISDEF 0x8		/* Comment was included in listing */
#define T_COM_ERASE 0x10	/* Comment marked for deletion */

#define T_SYM_TYPE   0xF	/* Type field mask */
#define T_SYM_HP     0x1	/* Entry is supported */
#define T_SYM_NOHP   0x2	/* Entry is not supported */
#define T_SYM_REVDEP 0x3	/* Entry is version dependant */
#define T_SYM_LNAME  0x4	/* Entry is local but named */
#define T_SYM_ROMP   0x5	/* xNAME or NULLNAME */
#define T_SYM_ISDEF 0x10	/* Symbol listing includes definition */
#define T_SYM_ERASE 0x20	/* Symbol marked for deletion (during merge) */
#define T_SYM_LOCAL 0x40	/* Symbol is local */

#define T_FMT_TYPE  0xF		/* Format type field mask */
#define T_FMT_RPL   0x1		/* Format is `r', form is NULL */
#define T_FMT_CODE  0x2		/* Format is `c', form is NULL */
#define T_FMT_VAR   0x3         /* Named variable */
#define T_FMT_HASH  0x4		/* Hash table */
#define T_FMT_LINK  0x5		/* Link table */
#define T_FMT_LIB   0x6		/* Library without prolog */
#define T_FMT_CRC   0x7		/* Library CRC */
#define T_FMT_AUTO  0x10	/* Format is auto-generated */

#define T_MACRO	   0		/* Unused, token type */

#define SEMI		0x0312b
#define LONG_INT	0x02614
#define FLASHPTR	0x026ac
#define SYSTEM_BINARY	0x02911
#define REAL_NUMBER	0x02933
#define LONG_REAL	0x02955
#define COMPLEX		0x02977
#define LONG_COMPLEX	0x0299d	
#define CHARACTER	0x029bf	
#define ARRAY		0x029e8	
#define LINKED_ARRAY	0x02a0a	
#define STRING		0x02a2c	
#define BINARY_INT	0x02a4e	
#define LIST		0x02a74	
#define DIRECTORY	0x02a96	
#define ALGEBRAIC	0x02ab8	
#define UNIT		0x02ada	
#define TAGGED		0x02afc	
#define GROB		0x02b1e	
#define LIBRARY		0x02b40	
#define BACKUP		0x02b62	
#define LIBRARY_DATA	0x02b88	
#define ACPTR		0x02baa
#define EXT1            0x02baa	
#define EXT2	        0x02bcc	
#define EXT3	        0x02bee	
#define EXT4            0x02c10
#define PROGRAM		0x02d9d	
#define CODE		0x02dcc	
#define GLOBAL_NAME	0x02e48	
#define LOCAL_NAME	0x02e6d	
#define ROMPTR	        0x02e92

#define IS_HEX(C)   (hextoi_map[(C) & 0xFF] != (char)-1)
#define HEX_TO_I(C) hextoi_map[(C) & 0xFF]
#define I_TO_HEX(D) hexmap[(D) & 0x0F]


/* 64-bit real. Uses quite a deal of space, but is only used
 * for various data conversions, never really stored anywhere.
 */
typedef struct {
  int x, mr, m, m1, s;
}
hp_real;

typedef hp_real hp_longreal;

#ifndef TRUE
#define FALSE 0
#define TRUE (!FALSE)
#endif

/* ********************************************************************** */
/* *		Provided global variables				* */
/* ********************************************************************** */

/* Options */
extern int
     opt_gx,		/* Binary is for GX */
     opt_hp49g,		/* HP49G style banks */
     opt_source,	/* Source-code like disassembly */
     opt_pass1,		/* Run pass 1 */
     opt_formats,	/* Run pass F */
     opt_debug,		/* Output debug info */
     opt_locals,	/* Local symbols */
     opt_code,		/* Include code in disassembly */
     opt_startpt,	/* Starting address */
     opt_endpt,		/* End address */
     arg_entry,		/* Arg. number of entry in cmdline */
     opt_entry,		/* Start entry */
     opt_entrylen,	/* Lenght exists */
     opt_entryend,	/* End entry exists */
     opt_symbols,	/* Symbols in code, or symbols in comments */
     opt_symdef,	/* Include defs of ref. unsupported symbols */
     opt_symdef_all,	/* Include defs of all referenced symbols */
     opt_opcode,	/* Include opcodes/addresses in listing */
     opt_printml,       /* Include machine language in listing */
     opt_comments,	/* Generate auto comments */
     opt_commentcol,	/* Comment column */
     opt_alonzo,	/* Alonzo-style formatting */
     opt_xref,		/* Include full XREF */
     opt_hp,		/* Supported entries only? */
     opt_jumpify,	/* Jumpify */
     opt_intelligent,	/* Follow code */
     opt_comref,	/* Comment susp. references */
     opt_groblast,	/* Draw grob after source */
     opt_drawgrob;	/* Draw grob */

/* Various code related variables */
extern int
     branch,		/* Last instruction forces branch? */
     gosub,		/* Was last instruction a subroutine call? */
     jumpaddr,		/* Last target address of a call/jump */
     last_cpc,		/* Last address of a C=PC */
     last_apc;		/* Last address of a A=PC */

/* Various rpl related variables */
extern int
     mode_rpl,		/* Old print mode */
     new_mode_rpl,	/* Print mode for current instruction */
     mode_single_op,	/* Print 1 instruction on line only */
     mode_fixed_line,	/* Print fixed number of cmds on line */
     fixed_ops,		/* Number of cmds on line in above mode */
     mode_endcode,	/* Must print ENDCODE? */
     mode_dispatch,	/* Print fixed no of obs on lines */
     dispatch_obs,	/* Number of obs allowed on line */
     dispatch_level,	/* comp_level of dispatcher */
     dispatch_lines,	/* line counter for ending dispatch mode */
     mode_imaginary_composite, /* RPL outside composites */
     rpl_comp_level,	/* Composite embedding depth */
     rpl_new_ind_level,	/* RPL indent level for next instruction */
     rpl_ind_level,	/* Current RPL indent level */
     source_ind;	/* Current source indent level */

extern int
     libnum,	        /* Current library number */
     libend,		/* Current library end */
     hashloc,		/* Location of hash table */
     linkloc;		/* Location of link table */

/* Various ROM related variables */
extern int
     pass,		/* Current pass (PASS1 or PASS2) */
     pc,		/* Current location */
     org_pc,		/* PC at start of instruction */
     baseaddr,		/* Base address of core file */
     ram_base,		/* RAM base address in SX */
     coresize;		/* Size of .core */

extern FILE 
     *core_file;	/* File used to access .core */

/* Misc variables */
extern int
     lineno,		/* Current input file line number */
     opt_lines,		/* Line numbers in errors and warnings */
     ncomments,		/* # of comments in .comments */
     nlocals,		/* # of local symbols from pass 1 */
     nsymbols,		/* # of symbols in .symbols */
     nmacros,		/* # of macros in .macros */
     nformats,		/* # of formats in .formats */
     nautoformats,	/* # of auto-generated formats */
     fmtseq,		/* Sequence # to use */
     fmtexpire_tag,	/* Format expire tag */
     format_added;	/* True if new format was added during PASSF */

extern char
     hextoi_map[0x100],
     hexmap[];

/* Data tables */
extern struct symbol
     *symroot,		/* Symbol chain start */
     **symref,		/* Symbol search/sort ref array */
     **nextsym,		/* Next symbol to expect */
     *comroot,		/* Comment chain start */
     **comref,		/* Comment search/sort ref array */
     **nextcom,		/* Next comment to expect */
     *macroot,		/* Macro chain start */
     **macref,		/* For sorted reference */
     *fmtroot,		/* Format chain start */
     **fmtref,		/* For sorted reference */
     **nextfmt,		/* Next format directive */
     **curfmtref;	/* Current format ref */

extern struct format
     *curfmt,		/* Current format active */
     *prevfmt;		/* Previous format */

/* Output strings */

extern char
     l_code[],			/* List buf: code */
     *l_codep,			/* Pointer to above */
     l_instr[],			/* List buf: instr */
     l_comment[],		/* List buf: comment */
     *l_commentp,	        /* Pointer to above */
     l_source[],		/* List buf: source line */
     *l_sourcep;		/* Pointer to above */


/* ********************************************************************** */
/* *		Global functions provided by misc.c			* */
/* ********************************************************************** */

extern void
     init(), set_pc();
extern int
     determine_pc(),
     fetch_unibbles(), fetch_nibbles(),
     get_unibbles(), get_pcunibbles(), get_nibbles(), get_pcnibbles();
extern char
     *make_hexstr(), *make_asc(), *make_ascstr(), *make_strline(),
     *make_grobstr(), *make_hexstr_rev();

extern char
     *addrtohex(), *realtos(), *itobmask(),
     *strdup(), *local_id();
extern 
     hexstrtoi(), anystrtoi(), intstrtoi();

extern void
     load_comments(), comment(), force_comment();

extern void
     load_symbols(), add_local_symbol();
extern
     comcmp(), symcmp();
extern char
     *cksymbolic(), *symbolic();


/* ********************************************************************** */
/* *		Global functions provided by output.c			* */
/* ********************************************************************** */

extern void
     error(), saderror(), sighandle(), warning(), disable_intr();

extern void
     separate(), print_source(), print_line(), 
     print_symdefs(), print_xref(), print_comfile(), print_symfile();

extern char
     *byspace();


/* ********************************************************************** */
/* *		Global functions provided by code.c			* */
/* ********************************************************************** */

extern void
     decode_instr();

/* ********************************************************************** */
/* *		Global functions provided by rpl.c			* */
/* ********************************************************************** */

int
     prologp(), force_prologp(), skipob(), is_header();

void
     decode_variable(), decode_linktable(), decode_hashtable(),
     decode_romlibrary(), decode_rpl(), decode_crc(), decode_data();

#endif


