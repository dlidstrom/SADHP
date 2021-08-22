* GNU Emacs AND sad.el

   The sad-mode facilitates interactive exploration of a core. First
edit sad.el and the runfile variables to point to sad, xsym, and xcom
as appropriate. (Default is according to the current search path.)
Load sad.el and do M-x sad. Emacs will first prompt for a range before
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
*	C-c C-d		Remove format.
*	C-c j		Join (see xfmt -j) format file.

	M-;		Add comment, or reindent current comment, as
			appropriate.
*	M-LF		Continue comment on next line.


	C-c s		View value of symbol under point.
	C-c .	/ M-.	Move to symbol definition. (Current buffer only)
	C-c ,	/ M-,	Move to next definition of same symbol, if any.

	C-c M		Edit macros database.
	C-c F		Edit formats database.
	C-c S		Edit symbols database.
	C-c C		Edit comments database.

	C-c e	/ C-x `	Move to line of next error in *SAD Output*.


	C-c C-c		Call on xcom to extract information,
			and redisassemble. Any errors or warnings
			go into the *SAD Output* buffer.


   After C-c C-c an attempt is made at approximately preserving the
current position, so don't be too suprised if the cursor moves a
couple of lines. The window is also recentered around the new point.

   The range is indicated in the mode line, and also makes the default
file name. Should you prefer some other file name, you can change the
variable *sad-default-file-name* in sad.el.

Please do not edit sad.el options unless you're absolutely sure what
you're doing.
