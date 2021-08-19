|
|	piece.030.s
|
|	CONFIDENTIAL
|	Copyright (c) 1988 NeXT, Inc. as an unpublished work.
|	All Rights Reserved.
|
|	Created Leo  06Jan88
|
|	A little subset of object-oriented programming for
|	our very own!
|
|	The situation is that we have some number of functions
|	that are identical for two kinds of structures: BitPieces
|	and DivPieces.  We want to call one of two procedures,
|	the BitPiece version or the DivPiece version, based on
|	the first two bytes of the structure. 'b' means a 
|	BitPiece, 'd' means a DivPiece.
|
|	Modified:
|	26Mar88  Leo  Make PieceCompositeFrom and PieceCompositeTo
|	              dispatch based on contents of CompositeOperation
|	28Feb89  Ted  Added Several routines.
|	22Apr89  Ted  Alpabetized methods, shortened type to uchar and
|		      changed temp lables to reflect method names.
|	11Jan90  Ted  Added PieceApplyBoundsProc.
|	09May90  Ted  Removed PieceCopyTree.
|	24Jun90  Ted  Removed PieceApplyBoundsProc.
|
	.text
||
	.globl	_PieceAdjust
	.globl	_DPAdjust,_BPAdjust
_PieceAdjust:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PAT
	jmp	_BPAdjust
_PAT:
	jmp	_DPAdjust
||
	.globl	_PieceApplyBoundsProc
	.globl	_DPApplyBoundsProc,_BPApplyBoundsProc
_PieceApplyBoundsProc:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PABP
	jmp	_BPApplyBoundsProc
_PABP:
	jmp	_DPApplyBoundsProc
||
	.globl	_PieceApplyProc
	.globl	_DPApplyProc,_BPApplyProc
_PieceApplyProc:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PALP
	jmp	_BPApplyProc
_PALP:
	jmp	_DPApplyProc
||
	.globl	_PieceBecomeDivAt
	.globl	_DPBecomeDivAt,_BPBecomeDivAt
_PieceBecomeDivAt:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PBDA
	jmp	_BPBecomeDivAt
_PBDA:
	jmp	_DPBecomeDivAt
||
	.globl	_PieceCompositeFrom
	.globl	_DPCompositeFrom,_BPCompositeFrom
_PieceCompositeFrom:
|
| Switches on cop->dst->type.
|
	movl	sp@(4),a0
	movl	a0@,a0
	cmpb	#0x64,a0@
	beqs	_PCF
	jmp	_BPCompositeFrom
_PCF:
	jmp	_DPCompositeFrom
||
	.globl	_PieceCompositeTo
	.globl	_DPCompositeTo,_BPCompositeTo
_PieceCompositeTo:
|
| Switches on cop->src->type.
|
	movl	sp@(4),a0
	movl	a0@(0x04),a0
	cmpb	#0x64,a0@
	beqs	_PCTO
	jmp	_BPCompositeTo
_PCTO:
	jmp	_DPCompositeTo
||
	.globl	_PieceDivideAt
	.globl	_DPDivideAt,_BPDivideAt
_PieceDivideAt:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PDA
	jmp	_BPDivideAt
_PDA:
	jmp	_DPDivideAt
||
	.globl	_PieceFindPieceBounds
	.globl	_DPFindPieceBounds,_BPFindPieceBounds
_PieceFindPieceBounds:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PFPB
	jmp	_BPFindPieceBounds
_PFPB:
	jmp	_DPFindPieceBounds
||
	.globl	_PieceFree
	.globl	_DPFree,_BPFree
_PieceFree:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PF
	jmp	_BPFree
_PF:
	jmp	_DPFree
||
	.globl	_PieceMark
	.globl	_DPMark,_BPMark
_PieceMark:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PM
	jmp	_BPMark
_PM:
	jmp	_DPMark
||
	.globl	_PieceObscureBecause
	.globl	_DPObscureBecause,_BPObscureBecause
_PieceObscureBecause:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_POB
	jmp	_BPObscureBecause
_POB:
	jmp	_DPObscureBecause
||
	.globl	_PieceObscureInside
	.globl	_DPObscureInside,_BPObscureInside
_PieceObscureInside:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_POI
	jmp	_BPObscureInside
_POI:
	jmp	_DPObscureInside
||
	.globl	_PiecePrintOn
	.globl	_DPPrintOn,_BPPrintOn
_PiecePrintOn:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PPO
	jmp	_BPPrintOn
_PPO:
	jmp	_DPPrintOn
||
	.globl	_PieceRevealBecause
	.globl	_DPRevealBecause,_BPRevealBecause
_PieceRevealBecause:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PRB
	jmp	_BPRevealBecause
_PRB:
	jmp	_DPRevealBecause
||
	.globl	_PieceRevealInside
	.globl	_DPRevealInside,_BPRevealInside
_PieceRevealInside:
	movl	sp@(4),a0
	cmpb	#0x64,a0@
	beqs	_PRI
	jmp	_BPRevealInside
_PRI:
	jmp	_DPRevealInside
||

