
* SYNOPSIS, SADHP 1.08

This is the README file for SAD, the Saturn Disassembler package. SAD comes
with no documentation at this time, other than this file.

SADHP is a package currently consisting of
	sad	- the disassembler
	port	- file loader
	uport	- symbols saver
	xcom	- comments extractor	(No GX support!)
	xsym	- symbols extractor	(No GX support!)
	xfmt	- formats extractor	(No GX support!)

Also provided is sad.el for emacs that offers easy studying of ROM.
(As it uses xcom, xsym and xfmt: No GX support!)

The purpose of SAD is to let you disassemble Saturn Machine Language (ML)
and RPL code, edit it, and maintain databases of symbols, comments,
formats and macros.  The formats database contains information
directing the disassembler to either ML, RPL, or Data, the latter of
which may be complex nested structures. The Macros database contains
nibble patterns for various common idioms.

SADHP was modified from SAD with the permission of Jan Brittenson.

* INSTALLATION

SAD does not provide "make install" command yet. To ease installing SAD you
should unzip the package in a directory called: ~/Hp/Sad. Otherwise you will
have to edit sad.el, all the shell scripts etc which is not recommended.

- Type "make" in the ~/Hp/Sad/Source directory
- Type "make clean" to delete the object files
- Move the shell scripts to your ~/bin: "mv scripts/* ~/bin"
- Type "rehash" to add the shell scripts to your command set

To install sad.el do the following:

- Add sad.el to your elisp load path. For example this will add ~/elisp to
  your load path, and your can then move sad.el into ~/elisp/sad.el

     (setq load-path (append (list (expand-file-name "~/elisp")
                                   nil ) load-path ))

- Add the following line to your .emacs:

	(autoload 'sad "sad" "Disassemble HP48 memory" t)


* DISASSEMBLING

Disassembly is done with the `sad' command:

		sad [flags] { name {len} | start end }

        where

	start,end	Hex addresses of first and last instructions.
	name		Name of disassembly startpoint entry.
	len		Optional lenght for entry disassembly

	flags, 		A set of flags, always bundled up as one argument.
		a	Assembler format, i.e. PC and opcode fields are
			suppressed.
		A	RPL++ format. SADHP tries to produce a valid source
			for RPL++ assembler, but this may not always work.
			Disables options "acsLM1"
		c	Suppression of disassembler comments.

		s	Symbolic addresses are moved to the comments.

		e	Disassembles named entry. For non-rpl objects
			skipover cannot be used to determine end address of
			disassembly, thus SADHP uses the next entry in the
			.symbols file to determine it.
		d	The supplementary definition of symbols known,
			referenced, but not otherwise defined in the
			output, is suppressed. Unsupported symbols are
			marked notified.
			Note: unsupported symbol <> unsupported entry.

		f	Keep repeating the F pass until no further
			formatting information can be colleced. Write
			output to formats.out.

		1	One pass only. Skip local symbols. Independent
			of the f flag.

		L	Don't generate local symbols (globals only).

		h	HP mode. Disassembler uses only supported entries.
			Here supported means the latest list of supported
			entries provided by HP which is available from
			hpcvbbs.cv.p.com:/pub/quick/48entry.zip
			Note that HP doesn't 'support' all user level
			commands :)

		C	Don't output any code. Useful if all you want
			is a cross reference (see -x below), or collect
			formatting information.

		M	Don't output machine language.

		x	A cross reference is added at the end, as
			comments with symbols and addresses in the
			disassembly where they are referenced.
		 	
		z	Alonzo mode. PC and opcode fields are printed
			slightly differently. The initial org instruction
			is suppressed. Note: This is remains of the old SAD
			versions that used 2 output formats. May change in the
			future to mean Alonzo mnemonics.

		G       Grobs are not drawn

		g	Grobs are drawn after source lines. A maximum of 68
			pixels are drawn on each line.

		X	Disassembles G/GX code. By default SADHP assumes the
			code is for SX.

		j	Jumpify. Uses informative local labels when possible.
			For example GOVLNG =GETPTR may be labeled "L_GETPTR"

* NOTES

"sad -adx 0 7ffff > hp48.src" will disassemble entire ROM with symbol and
cross reference tables, but be warned, the filesize will be several megabytes.
If you really want the entire disassembly though, I suggest using -j alone.

Use quotation for names with special characters, Unix example:

		sad -e '>R'

* PORT

	Usage: port filename

Port will convert a HP48 binary to a form (.port1) suitable for SADHP. Also
data files associated with the filename and stored to the DisAss directory
are copied to Sad directory for use of SADHP as follows:

	filename.f	--> .formats1
	filename.s	--> .symbols1
	filename.c	--> .comments1

If filename.f does not exist stdformats file is copied instead.

Port corresponds to loading a file to an empty, not merged RAM card in slot
1. Port assumes the file is a valid HP48 binary and thus has a binary
download header. The first 16 nibbles are always stripped away to remove the
header data, no check is made to see if the header is actually in there.


* UPORT

	Usage: uport filename

Uport will save existing .symbols1, .formats1 and .comments1 files to the
DisAss directory for further use.


* FILES

`.core' consists of binary raw data, where each byte corresponds to one
nibble. The upper half is reserved for other purposes, but currently unused.
Address 0 corresponds to offset 0. The full memory dump from 0 to 7FFFF
should take 524288 bytes.

'.core.gx' is optional and should contain the ROM dump for G/GX. If no SX ROM
dump is present then the G/GX dump should be named '.core', SADHP will
automatically recognize the ROM version from the start of the dump. See the
file romdump.doc if you have no ROM dump.

Both core files can be missing but then only HP48 binary files can be
disassembled. This can be done by first converting the file with port, then
using the start address 80000 for disassembly. SADHP chooses the disassembled
file according to the start address as follows:

	Start address	File

	0     - 7FFFF	.core	(.core.gx if option -X)
	80000 - BFFFF	.port1
	C0000 -	FFFFF	.port2

.port2 is provided mainly for disassembling address dependant libraries
starting at C0000, but is is also useful trick to keep some often
disassembled program in .port2 so that port (.port1) can be used to
disassemble other files.

`.symbols' consists of lines of the following format:

		<value>:<symbol>
		<value>=<symbol>
		<value>,<symbol>

Example:	03188=DSKTOP

Symbols marked with "=" are supported by HP. The .symbols file included
contains all the supported entries from the latest entries.a published by HP.
You can find it from hpcvbbs:/pub/quick/48entry.zip. User level entry names
are those used by HP in USRLIB.EXE, but not necessarily listed in entries.a.

Symbols marked with "," are unsupported but have not moved, symbols marked
with "," are at version dependant addresses. No guarantee is given about the
correctness of provided symbols lists, they were compliled after a quick
comparison with rev E and M ROMs.

For addresses over FFFFF the corresponding symbols is interpreted as a ROMPTR
name. In this case the address is interpreted as the ROMPTR body. SADHP uses
this feature to comment possible ROMPTRs in the disassembled object. Standard
.symbols file contains (some) unsupported names for the inbuilt library F0.
No such names are provided in .symbols.gx.

Example:	10F0005,Meta<-->+
		defines ROMPTR 0F0 005 name to be Meta<-->+

.symbols (or .symbols.gx) is always loaded in. Also .symbols1 or .symbols2 is
loaded if disassembly start address is in the corresponding range.

When SADHP comments labels used in code following convention is used:
	#address marks a supported entry
	!address marks an unsupported entry that has not moved
	!!address marks a version dependant entry


`.comments' ('comments.gx') is similar to .symbols:

		<address>=<comment string>
		<address>:<comment string>

Example:	05B79=Allocate string

   Several comments may be bound to the same address, in which case
they appear in the specified order. Here "=" and ":" reflect whether
the comment is considered a `major comment' or a mere `minor' one.
Major comments are comments put on a line of their own, whereas minor
comments are appended to the right of the code. "=" signals a major
comment, and ":" a minor. The semicolon is implicit, and not included.

   During disassembly, at any given address, all major comments are
output first, follwed by any symbol definitions, and then code with
minor comments appended to their right. The file is not ordered.

`.formats' ('formats.gx) contains disassembly formatting information, mostly
related to correctly decoding data, synchronization and indentation. The
directives can be divided into three categories: Machine Language (ML), RPL,
and Data. The file constists of entries of the form:

		<address>:<format>

specifying that from <address> and on, <format> is to be active. If
during disassembly <address> is about to be passed, the disassembler
will back up to <address>. This behaviour is called `synchronization,'
and is performed even if an identical format was previously in effect.
For RPL and ML, <format> is either `r' or `c' respectively, and may
not be nested or combined with or within Data format specifications.

The syntax for Data format specifications is:

		[<repeat>]<formatchar>[<width>]
	or	<format>[,<format>]
	or	[<repeat>](<format>)

   Where <repeat> and <width> are decimal integers. Commas (,) are
used to separate sequences of formats to be used sequentially.

   The format character <formatchar> is one of the following. `R'
refers to the repeat count, and `w' to the width.

   x	Hex	R words of W nibbles in hexadecimal.
   y    Hex	R REL(W) statements
   d	Dec	R words of W nibbles in decimal.
   o	Oct	R words of W nibbles in octal.
   a	Ascii	R sequences of W characters.
   s	String	R sequences of characters whose lengths are determined by
		a W-nibble word preceding the sequence, minus W.
   S    String  R sequences of characters whose lengths are determined by
		a W-nibble word preceding the sequence.
   v	Vector	R sequences of nibbles presented in hex, whose lengths
		are determined by a W-nibble word preceding the sequence,
		minus W.
   V	Vector	R sequences of nibbles presented in hex, whose lengths
		are determined by a W-nibble word preceding the sequence.
   w	Word	R 64-bit words presented in floating point, RPL style.
   i    ML	One machine language instruction.
   g    Grob	Draws R grob lines of width W (nibbles!!)
		If R <> 1 an empty line is output after each 'block' of
		grob lines.

Examples:

		5b79:c
		2a2b4:r
		2a2b4:x5,w
		7A32B:x1,5g2		(Small font with width data)

[Note: if the example above were actually used, the format effective at
2a2b4 would unpredictably be either one of the two conflicting ones.]

Please note that sad requires a non-empty formats file. Example .formats1 file
is provided containing the line:
		80000:r
Also the last line must end at newline to be recognized.

The provided formats files are quite complete for both rev E and M. Missing
are atleast the formats for the polynomial coefficients embedded in the
machine language floating point routines.

`.macros' contains pairs of patterns and macro definitions. The file consists
of entries of the form:

		<length>,<pattern>:<definition>

   Where <length> is the length of the pattern, <pattern> the pattern
data, and <definition> a string to be expanded. The left-hand side of
the colon (:) is referred to as the `tag.' The definition is the
resultant strings, possibly with embedded expansion directives. These
start with a percent sign (%), are optionally followed by width (W)
and adjustment (A) terms, and end in a directive character. The
interpretation, if any, of w and a is directive dependent.

General:
		%[<w>[,<a>]]<d>

Directives:
   %    %       Inserts % into output
   x	Hex	W (default 5) nibbles as hex digits, or as a symbol.
   d	Dec	W (default 5) nibbles as a decimal word.
   o	Oct	W (default 5) nibbles as an octal word.
   b	Bin	W (default 5) nibbles as a binary word.
   w	Word	64-bit word as a floating-point word, RPL fashion.
   l	Long	84-bit word as a long floating-point word, RPL fashion.
   a	Ascii	W (default 1) characters.
   s	String  W (default 1) byte word specifying the string length
		in bytes, minus A (default 0).
   S	String  W (default 1) byte word specifying the string length
		in characters.
   v	Vector	W (default 2) nibble word specifying the vector length,
		minus A (default 0), presented as hex digits.
   i	Instr.	W nibble (default 5) word minus 4 minus A specifies a length
		in nibbles to be disassembled as ML. Expands to the
		word content minus A, in decimal. Returns to previous
		format after ML of the given length has been disassembled.
   I	Instr	Override current format with ML (format `c').

   z	Skip	Skip (advance) W nibbles.

   +	Begin	Designate beginning of new block.
   -	End	Designate end of block.
   e	End	Same.
   
   =	Equal	Assert that the following W nibbles are A.

Examples:

		5,2a2c:$ "%5,5s"
		5,2a4e:HXS %5,5v
		5,2933:%% %w
		5,2d9d:::%+
		5,312b:;%-
		5,2e48:ID %2S
		5,2dcc:CODE %5,1i

New sad versions now have default disassembler routines for each object type
so all object type macros have been removed from .macros as being
unnecessary.  Since version 1.04 new indentation macros were installed for
formatting output when -A option is specified. These macros handle mainly
ITE, case, DISPATCH etc words that have common structures in user source
code. These macros are subject to change and thus are not documented here.

Note:

   In SADHP macros are used only during RPL disassembly, and are
restricted to 5-nibble sequences. You may, however, define macros with
pattern tags of any length up to 8 nibbles, they will merely be
ignored.

* EXAMPLES

Here are some example commands:

sad -e xDUP
sad -ae DUP
sad 0 3fff |less
sad -e CROSSGROB
sad -Ae DoKeyOb

To find out if a program uses unsupported entries try:
port filename
sad -Cd 80000 fffff |less

* GNU Emacs AND sad.el

   The sad-mode facilitates interactive exploration of a core. If you have
added the sad.el autoload command to your .emacs file you can start sad in an
emacs buffer by typing M-x sad. Emacs will first prompt for a range before
setting up a new buffer and disassembling. The range format is

		<from>-<to>

	   where	     are
		from,to		addresses in hexadecimal.



   While in a SAD buffer, the following key bindings are in effect.
C-c is the conventional "special mode prefix."

	C-c d		Redisassemble.
	C-c r		Set new range and redisassemble.
	C-c q		Quit current buffer.
	C-c n		Set up new buffer with new range.
	C-c o		Set up new buffer with new range in a new window.

	C-c v		View currently active format.
	C-c f		Change format. (Default is current PC)

	M-;		Add comment, or reindent current comment, as
			appropriate.

	C-c s		View value of symbol under point.
	C-c .	/ M-.	Move to symbol definition. (Current buffer only)
	C-c ,	/ M-,	Move to next definition of same symbol, if any.

	C-c M		Edit macros database.
	C-c F		Edit formats database.
	C-c S		Edit symbols database.
	C-c C		Edit comments database.

	C-c e	/ C-x `	Move to line of next error in *SAD Output*.


	C-c C-c		Call on xsym and xcom to extract information,
			and redisassemble. Any errors or warnings
			go into the *SAD Output* buffer.

   After C-c C-c an attempt is made at approximately preserving the
current position, so don't be too suprised if the cursor moves a
couple of lines. The window is also recentered around the new point.

   The range is indicated in the mode line, and also makes the default
file name. Should you prefer some other file name, you can change the
variable *sad-default-file-name* in sad.el.

Please do not edit sad.el options unless you're absolutely sure what
you're doing. For example the symbol and comment extractors heavily depend on
finding PC fields from the text, thus for example options -a and -A are out
of the question. Currently the options used for sad are -j.


* REPORTING BUGS

   If you find a bug in SADHP, you should report it. But first, you
should make sure that it really is a bug, and that it appears in the
latest version of SADHP that you have.

   Once you have ascertained that a bug really exists, please mail me
a bug report. If you have a fix, please mail that as well!
Suggestions and `philosophical' bugs are equally welcome.

Please include the following:

	* The version number of SADHP
	* A description of the bug behaviour
	* A short script or `recipe' which exercises the bug

And mail it to mozgy@hic.hr

* DISTRIBUTION AND COPYRIGHT

Latest version of SADHP is available from

	http://www.hpcalc.org/pc/programming/sadhp108.tar.gz

   SADHP is distributed in the hope that it will be useful, but with
ABSOLUTELY NO WARRANTY; without even implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


Enjoy,
						-- Jan Brittenson 
						   bson@ai.mit.edu

** SADHP was modified from SAD with the permission of Jan Brittenson.

						-- Mika Heiskanen
						   mheiskan@vipunen.hut.fi
						-- Mario Mikocevic
						   mozgy@hic.hr
