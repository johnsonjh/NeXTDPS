; electric-ps-mode
; 
; an attempt at an automatic PS mode.
; If you don't like the indenting, ....
;        
; Copyright (c) 1983, 1984 -- Adobe Systems Incorporated.
; RCSID: $Header$    
; written by Andrew I. Shore
; for Gosling Emacs (#85S3) should be modifiable for Unipress Emacs
; 
; PostScript is a trademark of Adobe Systems Incorporated.

(defun 
    (fixup-} dot instabs
	   (if (eolp) (delete-white-space))
	   (setq instabs (bolp))
	   (setq dot (dot))
	   (insert-character (last-key-struck))
	   (save-excursion 
	       (backward-paren)	; get us to matching brace
	       (if instabs
		   (save-excursion descol
		       (setq descol (current-column))
		       (goto-character dot)
		       (to-col descol)))
	       (if (dot-is-visible)
		   (sit-for 5)
		   (progn (beginning-of-line)
			  (set-mark)
			  (end-of-line)
			  (message (region-to-string))))
	   )
    )
)

(defun 
    (compute-indent 
	(if (bolp)
	    (if (looking-at "[ \t]*{")
		(- (current-indent) 3)
		(current-indent))
	    (progn
		  (error-occured (re-search-forward "{[ \t]*"))
		  (current-column)
	    )
	)
    )
)

(defun 
    (fixup-{ comp col
	   (setq col (current-column))
	   (save-excursion 
	       (delete-white-space)
	       (if (bolp)
		   (progn 
			  (error-occured (backward-balanced-paren-line))
			  (setq col (+ (compute-indent) 3)))))
	   (to-col col)
	   (insert-character '{')
    )
)

	    
;+ ps-nl-indent
;-	properly indent the next line

(defun 
    (ps-nl-indent column
	(save-excursion
	    (error-occured (backward-balanced-paren-line))
	    (setq column (compute-indent))
	)
	(newline)
	(to-col column)
    )
)

;+ ps-fixup-indent
;-	fix up indentation of the current line

(defun    
    (ps-fixup-indent
	(save-excursion column lbrc
	    (beginning-of-line)
	    (delete-white-space)
	    (save-excursion
		(if (looking-at "}")
		    (progn 
			  (forward-character)
			  (backward-paren)
			  (setq column (current-column))
		    )
		    (progn
			  (setq lbrc (= (following-char) '{'))
			  (error-occured (backward-character))
			  (error-occured (backward-balanced-paren-line))
			  (setq column (compute-indent))
			  (if lbrc (setq column (+ column 3)))
		    )
		)
	    )
	    (to-col column)
	)
    )
)

;+ ps-fixup-region
;-	fix up indentation the region 

(defun
    (ps-fixup-region
	(save-excursion
	    (if (> (dot)(mark)) (exchange-dot-and-mark))
            (beginning-of-line)
	    (while (& (! (eobp)) (<= (dot)(mark)))
		   (ps-fixup-indent)
		   (next-line)
	    )
	)
	(message "Done!")
    )
)

;+ ps-region-around-proc
(defun 
    (ps-region-around-proc
	(beginning-of-line)
	(mark)
	(forward-paren)
	(end-of-line)
	(exchange-dot-and-mark)
    )
)

;+ ps-indent
;-	fix up this function
(defun 
    (ps-indent 
	(ps-region-around-proc)
	(ps-fixup-region))
)

;+ ps-comments
;-	do the right thing with Postscript comments %

(defun
    (ps-comments col
	 (if (eolp)
	     (progn 
		    (setq col (current-column))
		    (delete-white-space)
		    (if (!= (preceding-char) '\{')
			(to-col (if (bolp) col comment-column))
		    )
		    (setq left-margin comment-column)
;		    (setq right-margin 77)
;		    (setq prefix-string "% ")
    		    (if (bobp)
			(insert-string "%")
			(insert-string "% ")
		    )
	     )
	     (insert-character '%')
	 )
    )
)

(defun 
    (ps-cout			; comment-out the following lines
	    (prefix-argument-loop 
		(beginning-of-line)
		(insert-string "% ")
		(beginning-of-line)
		(next-line)
	    )
    )
)
;+ electric-ps-mode
;-	initialize postscript mode

(defun 
    (electric-ps-mode
	(remove-all-local-bindings)
	(if (! buffer-is-modified)
	    (save-excursion 
		(error-occured 
		    (goto-character 1000)
		    (re-search-reverse "^% last edit: ")
		    (beginning-of-line)
		    (set-mark)
		    (end-of-line)
		    (delete-to-killbuffer)
		    (insert-string 
			(concat "% last edit: "
				(users-login-name) " "
				(current-time)
			))
		    (setq buffer-is-modified 0)
		)
	    )
	)
	(local-bind-to-key "ps-comments" "%")
	(local-bind-to-key "fixup-{" "{")
	(local-bind-to-key "fixup-}" "}")
	(local-bind-to-key "forward-paren" "\e)")
	(local-bind-to-key "backward-paren" "\e(")
	(local-bind-to-key "ps-indent" "\ej")
	(local-bind-to-key "ps-nl-indent" "\^j")
	(local-bind-to-key "ps-fixup-indent" "\ei")
	(local-bind-to-key "ps-cout" "\e%")
	(setq mode-string "electric-PS")
	(use-abbrev-table "PS")
	(use-syntax-table "PS")
	(novalue)
    )
)

; set up postscript syntax table
(use-syntax-table "PS")
(modify-syntax-entry "()   (")	; ( and ) are matching parens
(modify-syntax-entry ")(   )")
(modify-syntax-entry "(]   [")	; [ and ] are matching parens
(modify-syntax-entry ")[   ]")
(modify-syntax-entry "(}   {")	; { and } are matching parens
(modify-syntax-entry "){   }")
(modify-syntax-entry "\\    \\"); \ is a quote character
(modify-syntax-entry "  {  %")	; % starts a comment
(modify-syntax-entry "   } \^j"); newline ends one
(modify-syntax-entry "w    -+!$.^&=_~:?*<>|a-zA-Z0-9,"); word chars
(modify-syntax-entry "     /	 "); , space and tab are non-word chars
(novalue)
