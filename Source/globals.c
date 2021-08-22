#include <stdio.h>
#include "globals.h"

/* Options */
int
     opt_gx = FALSE,		/* Binary is for GX */
     opt_source = FALSE,	/* Source-code like disassembly */
     opt_pass1 = TRUE,		/* Run pass 1 */
     opt_formats = FALSE,	/* Run pass F */
     opt_debug = FALSE,		/* Output debug info */
     opt_locals = TRUE,		/* Local symbols */
     opt_code = TRUE,		/* Include code in disassembly */
     opt_startpt=0x00000,	/* Starting address */
     opt_endpt=0x7FFFF,		/* End address */
     arg_entry,			/* Arg. number of entry in cmdline */
     opt_entry = FALSE,		/* Start entry exists */
     opt_entrylen = FALSE,	/* Lenght exists */
     opt_entryend = FALSE,	/* End entry exists */
     opt_symbols = TRUE,	/* Symbols in code, or symbols in comments */
     opt_symdef = FALSE,	/* Include defs of ref. unsupported symbols */
     opt_symdef_all = FALSE,	/* Include defs of all referenced symbols */
     opt_opcode = TRUE,		/* Include opcodes/addresses in listing */
     opt_printml = TRUE,        /* Include machine language in listing */
     opt_comments = FALSE,	/* Generate auto comments */
     opt_commentcol = COMMENT_COL,	/* Comment column */
     opt_alonzo = FALSE,	/* Alonzo-style formatting */
     opt_xref = FALSE,		/* Include full XREF */
     opt_hp=FALSE,		/* Supported entries only? */
     opt_jumpify=FALSE,		/* Jumpify */
     opt_intelligent=FALSE,	/* Follow code */
     opt_comref=TRUE,		/* Comment susp. references */
     opt_groblast=FALSE,	/* Draw grob after source */
     opt_drawgrob=TRUE,		/* Draw grob */
     opt_hp49g=TRUE;		/* HP49G style banks */

/* Various code related variables */
int
     branch=FALSE,		/* Last instruction forces branch? */
     gosub=FALSE,		/* Was last instruction a subroutine call? */
     jumpaddr=0,		/* Last target address of a call/jump */
     last_cpc=0,		/* Last address of a C=PC */
     last_apc=0;		/* Last address of a A=PC */

/* Various rpl related variables */
int
     mode_rpl=TRUE,		/* Old print mode */
     new_mode_rpl=TRUE,		/* Print mode for current instruction */
     mode_single_op=FALSE,	/* Print 1 instruction on line only */
     mode_fixed_line=FALSE,	/* Print fixed number of cmds on line */
     fixed_ops=0,		/* Number of cmds on line in above mode */
     mode_endcode=FALSE,	/* Must print ENDCODE? */
     mode_dispatch=FALSE,	/* Print fixed no of obs on lines */
     dispatch_obs=0,		/* Number of obs allowed on line */
     dispatch_level=0,		/* comp_level of dispatcher */
     dispatch_lines=0,		/* line counter for ending dispatch mode */
     mode_imaginary_composite=FALSE, /* RPL outside composites */
     rpl_comp_level = 0,	/* Composite embedding depth */
     rpl_new_ind_level = 0,	/* RPL indent level for next instruction */
     rpl_ind_level = 0,		/* Current RPL indent level */
     source_ind = 0;		/* Current source indent level */

int
     libnum = 0,	        /* Current library number */
     libend = 0,		/* Current library end */
     hashloc = 0,		/* Location of hash table */
     linkloc = 0;		/* Location of link table */

/* Various ROM related variables */
int
     pass = 0,			/* Current pass (PASS1 or PASS2) */
     pc = 0,			/* Current location */
     org_pc = 0,		/* PC at start of instruction */
     baseaddr = 0,		/* Base address of core file */
     ram_base = 0x70000,	/* RAM base address in SX */
     coresize = 0;		/* Size of .core */

FILE 
     *core_file;		/* File used to access .core */

/* Misc variables */
int
     lineno = 0,		/* Current input file line number */
     opt_lines = 0,		/* Line numbers in errors and warnings */
     ncomments = 0,		/* # of comments in .comments */
     nlocals = 0,		/* # of local symbols from pass 1 */
     nsymbols = 0,		/* # of symbols in .symbols */
     nmacros = 0,		/* # of macros in .macros */
     nformats = 0,		/* # of formats in .formats */
     nautoformats = 0,		/* # of auto-generated formats */
     fmtseq = 0,		/* Sequence # to use */
     fmtexpire_tag = 0,		/* Format expire tag */
     format_added = FALSE;	/* True if new format was added during PASSF */


char
     hextoi_map[0x100],
     hexmap[] = "0123456789ABCDEF";

/* Data tables */
struct symbol
     *symroot = NULL,		/* Symbol chain start */
     **symref = NULL,		/* Symbol search/sort ref array */
     **nextsym = NULL,		/* Next symbol to expect */
     *comroot = NULL,		/* Comment chain start */
     **comref = NULL,		/* Comment search/sort ref array */
     **nextcom = NULL,		/* Next comment to expect */
     *macroot = NULL,		/* Macro chain start */
     **macref = NULL,		/* For sorted reference */
     *fmtroot = NULL,		/* Format chain start */
     **fmtref = NULL,		/* For sorted reference */
     **nextfmt = NULL,		/* Next format directive */
     **curfmtref = NULL;        /* Current format ref */

struct format
     *curfmt = NULL,		/* Current format active */
     *prevfmt = NULL;		/* Previous format */

/* Output strings */

char
     l_code[5000],              /* List buf: code */
     *l_codep,			/* Pointer to above */
     l_instr[5000],		/* List buf: instr */
     l_comment[5000],		/* List buf: comment */
     *l_commentp,	        /* Pointer to above */
     l_source[5000],		/* List buf: source line */
     *l_sourcep;		/* Pointer to above */


