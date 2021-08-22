/* formats.h -- formats header
 */

#ifndef _FMT_CONFG_
#define _FMT_CONFG_

/* Each format description is an array of subdescriptions.
 * A subdescription may in turn refer to yet another subdescriptions.
 */

struct format_ent {
  short
    ftype;			/* Format type, see FT_xxx below */

  int
    width,			/* Format width, type dependent */
    repeat;			/* Format repeat factor */
    
  union {
    struct format
      *sf;			/* Reference to subdescription, if any */
  }
  t_v;
};


#define fmtdescr t_v.sf

struct format {
  short
    nformats;			/* Number of formats in .fmtspec[] */

  struct format_ent
    fmtspec[1];			/* Token declaration */
};


#define FT_DATA_MASK 0x3F
#define FT_HEX       0x1	/* Hex number */
#define FT_DEC       0x2	/* Decimal number */
#define FT_OCT       0x3	/* Octal number */
#define FT_BIN       0x4	/* Binary number */
#define FT_FLOAT     0x5	/* Decimal floating point */
#define FT_STR       0x6	/* String */
#define FT_STRLINE   0x7	/* String ending in 00 or newline */
#define FT_VSTR      0x8	/* Vectored string */
#define FT_VSTR2     0x9	/* Vectored string with no reduction */
#define FT_SPCHAR    0xA	/* Special character set for format 'z' */
#define FT_VHEX      0xB	/* Vectored hex data */
#define FT_VHEX2     0xC	/* Vectored hex with no reduction */
#define FT_INSTR1    0xD	/* One ml instruction */
#define FT_RPL1      0xE	/* One rpl instruction */
#define FT_GRBROW    0xF	/* One row of grob drawing */
#define FT_REL       0x10	/* REL(x) statement */
#define FT_CRC       0x11	/* 4 nibbles for CRC */

#define STRLINEMAX 40		/* Default maximum lenght of 'A' */

#define FT_CODE 0x40		/* ML code */
#define FT_RPL  0x80		/* RPL code */
#define FT_VAR 0x100		/* Named variable */
#define FT_HASH 0x200		/* Hash table */
#define FT_LINK 0x400		/* Link table */
#define FT_LIB 0x800		/* Library without a prolog */
#define FT_SUBD 0x1000		/* Identifies a subdescription */

extern void
  fmt_free(), load_formats(), fmt_print(),
  add_auto_format(), add_autos_to_fmttab(), print_format_file();

extern struct format
  *formatp();

extern struct symbol
  *fmt();

#endif
