;; sad.el - Saturn Disassembler mode for GNU Emacs
;;
;; SAD is not distributed by Free Software Foundation. Do not ask them
;; for a copy or how to obtain new releases. Instead, send e-mail to the
;; address below. SAD is merely covered by the GNU General Public
;; License.
;;
;; Please send your comments, ideas, and bug reports to
;; Jan Brittenson <bson@ai.mit.edu>
;;
;;
;; Copyright (C) 1990 Jan Brittenson.
;;
;; This file is part of SAD, the Saturn Disassembler package.
;;
;; SAD is free software; you can redistribute it and/or modify it under
;; the terms of the GNU General Public License as published by the Free
;; Software Foundation; either version 1, or (at your option) any later
;; version.
;;
;; SAD is distributed in the hope that it will be useful, but WITHOUT ANY
;; WARRANTY; without even the implied warranty of MERCHANTABILITY or
;; FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;; for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with SAD; see the file COPYING.  If not, write to the Free
;; Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
;;


; Default comment column
(defconst *sad-comment-column* 40)

; Format of file name of buffer, arguments are start and end addresses
(defconst *sad-file-format* "%05x-%05x")

; Runfiles and options
(defvar *sad-sad-runfile* "sad")
(defvar *sad-sad-options* "-j")
(defvar *sad-sad-format-options* "-df")

(defvar *sad-xsym-runfile* "xsym")
(defvar *sad-xsym-options* "-rsl")

(defvar *sad-xcom-runfile* "xcom")
(defvar *sad-xcom-options* "-rsl")

(defvar *sad-fmt-runfile* "xfmt")
(defvar *sad-fmt-view-options* nil)
(defvar *sad-fmt-set-options* "-r")
(defvar *sad-fmt-delete-options* "-rd")
(defvar *sad-fmt-join-options* "-rj")

(defconst *sad-macro-file* (expand-file-name "~/Hp/Sad/.macros"))
(defconst *sad-symbols-file* (expand-file-name "~/Hp/Sad/.symbols"))
(defconst *sad-formats-file* (expand-file-name "~/Hp/Sad/.formats"))
(defconst *sad-comments-file* (expand-file-name "~/Hp/Sad/.comments"))
(defconst *sad-fmt-joinfile* (expand-file-name "~/Hp/Sad/formats.out"))

; Don't clobber the rest.

(defvar sad-mode-line-range "no range")
(make-variable-buffer-local 'sad-mode-line-range)

(defvar sad-range nil)
(make-variable-buffer-local 'sad-range)

(defvar *sad-buffer-counter* 0)

(defconst *sad-hex-regexp* "#?\\([0-9a-fA-F]+\\)")

(defconst *sad-range-regexp*
  (format "\\`%s-%s\\'" *sad-hex-regexp* *sad-hex-regexp*))

(defconst *sad-no-range-error* "No active range.")

(defvar *sad-output-buffer* nil)
(defconst *sad-output-buffer-name* "*SAD Output*")

(defvar *sad-symbol* nil)

(defvar *sad-mode-keymap* nil)

(if (not *sad-mode-keymap*)
    (progn
      (setq *sad-mode-keymap* (make-keymap))
      (define-key *sad-mode-keymap* "\C-cr" 'sad-new-range)
      (define-key *sad-mode-keymap* "\C-c\C-c" 'sad-update)
      (define-key *sad-mode-keymap* "\C-cd" 'sad-disassemble)
      (define-key *sad-mode-keymap* "\C-cq" 'sad-quit)
      (define-key *sad-mode-keymap* "\C-cn" 'sad-new-buffer)
      (define-key *sad-mode-keymap* "\C-co" 'sad-new-buffer-other-window)
      (define-key *sad-mode-keymap* "\C-c." 'sad-find-symbol)
      (define-key *sad-mode-keymap* "\M-." 'sad-find-symbol)
      (define-key *sad-mode-keymap* "\C-c," 'sad-find-symbol-next)
      (define-key *sad-mode-keymap* "\M-," 'sad-find-symbol-next)
      (define-key *sad-mode-keymap* "\C-x`" 'sad-next-error)
      (define-key *sad-mode-keymap* "\C-c`" 'sad-next-error)
      (define-key *sad-mode-keymap* "\C-ce" 'sad-next-error)
      (define-key *sad-mode-keymap* "\C-cv" 'sad-view-format)
      (define-key *sad-mode-keymap* "\C-cf" 'sad-set-format)
      (define-key *sad-mode-keymap* "\C-cM" 'sad-edit-macros)
      (define-key *sad-mode-keymap* "\C-cS" 'sad-edit-symbols)
      (define-key *sad-mode-keymap* "\C-cF" 'sad-edit-formats)
      (define-key *sad-mode-keymap* "\C-cC" 'sad-edit-comments)
;      (define-key *sad-mode-keymap* "\C-c\C-d" 'sad-delete-format)
;      (define-key *sad-mode-keymap* "\C-cj" 'sad-join-formats)
      (define-key *sad-mode-keymap* "\C-cs" 'sad-view-symbol)
      ))

(defvar *sad-mode-syntax* nil)

(if (not *sad-mode-syntax*)
    (progn
      (setq *sad-mode-syntax* (make-syntax-table))
      (modify-syntax-entry ?\" "$" *sad-mode-syntax*)
      (modify-syntax-entry ?_ "_" *sad-mode-syntax*)
      (modify-syntax-entry ?+ " " *sad-mode-syntax*)
      (modify-syntax-entry ?- " " *sad-mode-syntax*)
      (modify-syntax-entry ?/ " " *sad-mode-syntax*)
      (modify-syntax-entry ?* " " *sad-mode-syntax*)
      (modify-syntax-entry ?^ " " *sad-mode-syntax*)
      (modify-syntax-entry ?@ " " *sad-mode-syntax*)
      (modify-syntax-entry ?% " " *sad-mode-syntax*)
      (modify-syntax-entry ?& " " *sad-mode-syntax*)
      (modify-syntax-entry ?= " " *sad-mode-syntax*)
      (modify-syntax-entry ?~ " " *sad-mode-syntax*)
      (modify-syntax-entry ?| " " *sad-mode-syntax*)
      (modify-syntax-entry ?! " " *sad-mode-syntax*)
      (modify-syntax-entry ?\? " " *sad-mode-syntax*)
      (modify-syntax-entry ?$ "_" *sad-mode-syntax*)
      (modify-syntax-entry ?{ "(}" *sad-mode-syntax*)
      (modify-syntax-entry ?[ "(]" *sad-mode-syntax*)
      (modify-syntax-entry ?} ")" *sad-mode-syntax*)
      (modify-syntax-entry ?] ")" *sad-mode-syntax*)
      (modify-syntax-entry ?; "<" *sad-mode-syntax*)
      (modify-syntax-entry ?\< "(>" *sad-mode-syntax*)
      (modify-syntax-entry ?> ")" *sad-mode-syntax*)
      (modify-syntax-entry ?\# " " *sad-mode-syntax*)))

; Major mode

(defun sad-mode ()
  "Major mode for the Saturn disassembler.

Mode key bindings:
\\{*sad-mode-keymap*}"

  (interactive)
  (setq major-mode 'sad-mode)
  (setq mode-name "SAD")
  (make-local-variable 'sad-sequence)
  (setq sad-sequence (format "[%d]" *sad-buffer-counter*))
  (use-local-map *sad-mode-keymap*)
  (set-syntax-table *sad-mode-syntax*)
  (setq mode-line-format
	'(" " sad-sequence " SAD:  %*%*  (" sad-mode-line-range
	  ")  ---- %p %-"))
  (make-local-variable 'comment-column)
  (setq comment-column *sad-comment-column*)
  (make-local-variable 'comment-start)
  (setq comment-start "* ")
  (make-local-variable 'comment-start-skip)
  (setq comment-start-skip "* ?")
  (make-local-variable 'comment-end)
  (setq comment-end  "")
  (make-local-variable 'comment-multi-line)
  (setq comment-multi-line t)
  (make-local-variable 'mode-specific-map)
  (setq mode-specific-map *sad-mode-keymap*)
  (buffer-enable-undo)
  (set-buffer-modified-p nil)
  (if (not *sad-output-buffer*)
      (setq-default *sad-output-buffer*
		    (get-buffer-create *sad-output-buffer-name*))))

; Ask for range, create new buffer, set sad-mode, and set new range.
; (sad) is an alias for (sad-new-buffer).

(defun sad ()
  "Enter sad-mode in new buffer."
  (interactive)
  (sad-new-buffer))

(defun sad-new-buffer (&optional range-prompt do-not-switch)
  "Start disassembly in new buffer."
  (interactive)
  (let ((range-arg
	 (read-string 
	  (if range-prompt
	      range-prompt
	    "New range:  ")
	  (if sad-range
	      sad-mode-line-range
	    ""))))
    (if (string-match *sad-range-regexp* range-arg)
	(progn
	  (setq-default *sad-buffer-counter* (1+ *sad-buffer-counter*))
	  (make-local-variable 'sad-buffer)
	  (setq sad-buffer
		(get-buffer-create
		 (format "*SAD Disassembly %d*" *sad-buffer-counter*)))
	  (if do-not-switch
	      (set-buffer sad-buffer)
	    (switch-to-buffer sad-buffer))
	  (sad-mode)
	  (sad-new-range range-arg))
      (error "Bad range, %s" range-arg))))


; Prompt for new range, parse it, and disassemble

(defun sad-new-range (&optional range)
  "Set new SAD disassembly range. Prompt for range and redisassemble."
  (interactive)
  (if (or (not (buffer-modified-p (current-buffer)))
	  (yes-or-no-p "Buffer modified, discard changes? "))
      (let* ((new-range
	      (if range range
		(let ((rrange
		       (read-string "Set range: "
				    (if sad-range
					sad-mode-line-range
				      ""))))
		  (if (string-match *sad-range-regexp* rrange)
		      rrange
		    (error "Bad range, %s" rrange)))))
	     (startaddr
	      (sad-parse-hex (substring new-range
					(match-beginning 1)
					(match-end 1))))
	     (endaddr
	      (sad-parse-hex (substring new-range
					(match-beginning 2)
					(match-end 2)))))
	(set-buffer-modified-p nil)
	(if (> startaddr endaddr)
	    (let ((tmp endaddr))
	      (setq endaddr startaddr
		    startaddr tmp)))
	
	(setq sad-mode-line-range (format "%05x-%05x" startaddr endaddr)
	      buffer-file-name (format *sad-file-format* startaddr endaddr)
	      sad-range (cons startaddr endaddr))
	(sad-disassemble))))
			  

; Discard buffer contents, execute disassemble command,
; and insert contents in buffer.

(defun sad-disassemble (&optional arg)
  "Disassemble locations defined by current range. Will prompt for
range if none defined in current buffer. The point after disassembly
is restored, roughly. Optional non-nil ARG means automagic format
generation as well."
  (interactive)
  (if (or (not (buffer-modified-p (current-buffer)))
	  (yes-or-no-p "Buffer modified, discard changes? "))
      
      (if (not sad-range)
	  (sad-new-range)
	(let ((pt (point))
	      (options (if arg
			   *sad-sad-format-options*
			 *sad-sad-options*)))
	  (erase-buffer)
	  (message "Disassembling...")

	  (if options
	      (call-process *sad-sad-runfile* nil t nil
			    options
			    (format "%x" (car sad-range))
			    (format "%x" (cdr sad-range)))
	    (call-process *sad-sad-runfile* nil t nil
			  (format "%x" (car sad-range))
			  (format "%x" (cdr sad-range))))
	  
	  (set-buffer-modified-p nil)
	  (goto-char pt)
	  (message "")))))

; Same as sad-new-buffer, but in other window

(defun sad-new-buffer-other-window ()
  "Start new disassembly in separate window."
  (interactive)
  (sad-new-buffer "New range in other window: " t)
  (let* ((buffer (current-buffer))
	 (pop-up-windows t))
    (pop-to-buffer buffer t)))


; Update tables, then redisassemble

(defun sad-update ()
  "Update tables and redisassemble."
  (interactive)
  (if (not sad-range)
      (error *sad-no-range-error*)
    (if (not (buffer-modified-p (current-buffer)))
	(message "(No changes made.)")
      (if (save-excursion
	    (save-excursion
	      (set-buffer *sad-output-buffer*)
	      (erase-buffer)
	      (insert "Symbol Extraction -\n")
	      (set-buffer-modified-p nil))

	    (message "Extracting symbol info...")
	    (call-process-region (point-min) (point-max)
				 *sad-xsym-runfile* nil
				 *sad-output-buffer*
				 nil
				 *sad-xsym-options*)
	    (save-excursion
	      (set-buffer *sad-output-buffer*)
	      (if (not (buffer-modified-p))
		  (erase-buffer)
		(insert "\nComment Extraction -\n")
		(setq compt (point))
		(set-buffer-modified-p nil)))

	    (message "Extracting comment info...")
	    (call-process-region (point-min) (point-max)
				 *sad-xcom-runfile* nil
				 *sad-output-buffer*
				 nil
				 *sad-xcom-options*)
	      
	    (set-buffer *sad-output-buffer*)
	    (if (not (buffer-modified-p))
		(delete-region compt (point-max)))
	    
	    (message " ")
	    (> (buffer-size) 0))
	  
	  (set-window-start (display-buffer *sad-output-buffer*) 1)
	
	(set-buffer-modified-p nil)
	(sad-disassemble)))))

; Parse hex, return as integer

(defun sad-parse-hex (hex-string)
  (let ((acc 0)
	(ppos 0)
	(pstop (length hex-string)))

    (while (< ppos pstop)
      (let ((one-digit (string-to-char
			(substring hex-string ppos (1+ ppos)))))
	(setq acc (+ (* acc 16)
		     (if (>= one-digit 97) ;a-f
			 (- one-digit 87)
		       (if (>= one-digit 65) ;A-F
			   (- one-digit 55)
			 (- one-digit 48)))))) ;0-9
      (setq ppos (1+ ppos)))
    acc))

; Quit

(defun sad-quit ()
  "Quit the current buffer."
  (interactive)
  (if (or (not (buffer-modified-p))
	  (yes-or-no-p "Buffer modified, really quit? "))
      (progn
	(set-buffer-modified-p nil)
	(kill-buffer (current-buffer)))))

;; Return a default symbol to search for, based on the text at point.

(defun sad-default-symbol ()
  (save-excursion
    (while (and (looking-at "\\sw\\|\\s_")
		(> (point) (point-min)))
      (backward-char 1))

    (if (> (point) (point-min))
	(forward-char 1))
    (let ((start (point)))
      (while (and (looking-at "\\sw\\|\\s_")
		  (< (point) (point-max)))
	(forward-char 1))
      (if (>= start (point))
	  nil
	(buffer-substring start (point))))))

; Ask for symbol name

(defun sad-ask-for (prompt default)
  (let ((answer
	 (read-string
	  (if default
	      (format "%s(default %s" prompt default)
	    prompt))))
    (if (string= answer "")
	default
      answer)))

(defun sad-ask-for-symbol (prompt)
  (sad-ask-for prompt (sad-default-symbol)))

; Find symbol

(defun sad-find-symbol ()
  "Find symbol definition."
  (interactive)
  (let* ((sym (sad-ask-for-symbol "Find symbol: "))
	 (symdef-pattern
	  (format "\\(^=%s[ \t]+EQU\\|[ \t]+=%s\\)" (regexp-quote sym) sym))
;         (format "\\(^\\|[ \t]+\\)%s[:=]" (regexp-quote sym)))
      (sym-point nil))
    (setq *sad-symbol* nil)
    (save-excursion
      (goto-char (point-min))
      (if (re-search-forward symdef-pattern (point-max) t)
	  (setq sym-point (point))
	(error "No definition of %s" sym)))

    (push-mark)
    (goto-char sym-point)
    (setq *sad-symbol* sym)))


; Find next definition of symbol with same name, if any

(defun sad-find-symbol-next ()
  "Find next definition."
  (interactive)
  (if (not *sad-symbol*)
      (error "No further definitions"))
  (let ((sym-point nil)
	(symdef-pattern
	 (format "\\(^=%s[ \t]+EQU\\|[ \t]+=%s\\)" *sad-symbol* *sad-symbol*)))
;	 (format "\\($\\|[ \t]+\\)%s[=:]" *sad-symbol*)))
    (save-excursion
      (if (re-search-forward symdef-pattern (point-max) t)
	  (setq sym-point (point))
	(setq *sad-symbol nil)
	(error "No further definitions")))

    (push-mark)
    (goto-char sym-point)))

; View symbol

(defun sad-view-symbol ()
  "View value of symbol."
  (interactive)
  (let* ((sym (sad-ask-for-symbol "View symbol: "))
	 (sym-pattern
	  (format "^%s[:=,]%s$" *sad-hex-regexp* (regexp-quote sym))))
    (save-excursion
      (set-buffer *sad-output-buffer*)
      (erase-buffer)
      (insert-file-contents *sad-symbols-file*)
      (goto-char (point-min))
      (if (re-search-forward sym-pattern (point-max) t)
	  (message
	   (format "%s = %s"
		   sym
		   (buffer-substring
		    (match-beginning 1)
		    (match-end 1))))
	(error "No symbol %s" sym))
      (erase-buffer))))

; Goto next error

(defun sad-next-error ()
  "Move to line of next update error or warning."
  (interactive)
  (goto-line
   (save-excursion
     (if (not *sad-output-buffer*)
	 (error "No more errors/warnings"))
     (set-buffer *sad-output-buffer*)
     (goto-char (point-min))
     (kill-line 1)
     (while (not (or (zerop (buffer-size))
		     (looking-at "^\\([0-9]+\\): .+")))
       (kill-line 1))
     (if (zerop (buffer-size))
	 (error "No more errors/warnings"))
     (string-to-int (buffer-substring (match-beginning 1)
				      (match-end 1))))))

; Determine default address

(defun sad-default-address ()
  (save-excursion
    (beginning-of-line)
    (if (re-search-forward "^[0-9a-fA-F]+" (point-max) t)
	(buffer-substring (match-beginning 0) (match-end 0)))))

; Ask for address, supplying default

(defun sad-ask-for-address (prompt)
  (sad-ask-for prompt (sad-default-address)))

; View format of address

(defun sad-view-format ()
  "View format of specific address."
  (interactive)
  (let* ((address (sad-ask-for-address "View for address: ")))

    (save-excursion
      (set-buffer *sad-output-buffer*)
      (erase-buffer)
      (set-buffer-modified-p nil)
      (if *sad-fmt-view-options*
	  (call-process *sad-fmt-runfile* nil t nil
			*sad-fmt-view-options* address)
	(call-process *sad-fmt-runfile* nil t nil address))

      (if (not (buffer-modified-p))
	  (message "(No format info for %s.)" address)
	(message "Format active at %s is %s"
		 address
		 (buffer-substring (point-min) (1- (point-max))))))))

; Set format

(defun sad-set-format ()
  "Set format of specific address."
  (interactive)
  (let* ((address (sad-ask-for-address "Set for address: "))
	 (intaddr (sad-parse-hex address))
	 (fmtprompt (format "Format to set at %s: " address))
	 (newfmt (read-string fmtprompt)))

    (if (save-excursion
	  (set-buffer *sad-output-buffer*)
	  (erase-buffer)
	  (set-buffer-modified-p nil)
	  
	  (if *sad-fmt-set-options*
	      (call-process  *sad-fmt-runfile* nil t nil
			     *sad-fmt-set-options* address newfmt)
	    (call-process  *sad-fmt-runfile* nil t nil address newfmt))
	  
	  (if (buffer-modified-p)
	      (progn
		(message "%s"
			 (buffer-substring (point-min) (1- (point-max))))
		nil)
	    t))
	
	(if (y-or-n-p "Redisassemble? ")
	    (sad-disassemble)))))

; Edit macros

(defun sad-edit-macros ()
  "Edit contents of macro definition file."
  (interactive)
  (find-file-other-window *sad-macro-file*))

; Edit formats

(defun sad-edit-formats ()
  "Edit contents of formats definition file."
  (interactive)
  (find-file-other-window *sad-formats-file*))

; Edit comments

(defun sad-edit-comments ()
  "Edit contents of comments definition file."
  (interactive)
  (find-file-other-window *sad-comments-file*))

; Edit symbols

(defun sad-edit-symbols ()
  "Edit contents of symbols definition file."
  (interactive)
  (find-file-other-window *sad-symbols-file*))


; Remove format

(defun sad-delete-format ()
  "Remove any format from specific address."
  (interactive)
  (let* ((address (sad-ask-for-address "Remove for address: "))
	 (intaddr (sad-parse-hex address)))

    (if (save-excursion
	  (set-buffer *sad-output-buffer*)
	  (erase-buffer)
	  (set-buffer-modified-p nil)
	  (call-process  *sad-fmt-runfile* nil t nil
			 *sad-fmt-delete-options* address)
	  (if (buffer-modified-p)
	      (progn
		(message "%s"
			 (buffer-substring (point-min) (1- (point-max))))
		nil)
	    t)

	(if (y-or-n-p "Redisassemble? ")
	    (sad-disassemble))))))

; Join formats

(defun sad-join-formats ()
  "Join formats."
  (interactive)
  (let* ((filename
	  (read-file-name "Join with file: " nil
			  *sad-fmt-joinfile* t)))
    (save-excursion
      (set-buffer *sad-output-buffer*)
      (erase-buffer)
      (set-buffer-modified-p nil)
      (call-process  *sad-fmt-runfile* nil t nil
			 *sad-fmt-join-options* filename)
      (if (buffer-modified-p)
	  (progn
	    (message "%s"
		     (buffer-substring (point-min) (1- (point-max))))
	    nil)
	t))))