/*****************************************************************************

    piece.c
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
    
    Created 23Apr86 Leo
    
    Modified:
    
    21Sep87  Leo  Absolute max on storage pools
    06Jan88  Leo  Straight C, removed old modified log
    26Mar88  Leo  CompositeOperation structure
    06Jun88  Erik Transformed into piece_dispatch
    05Dec89  Ted  Integratathon and ANSI C Prototyping
    11Jan90  Ted  Updated to WS 2.0 spec, added PieceApplyBoundsProc
    09May90  Ted  Removed PieceCopyTree()
    24Jun90  Ted  Optimized PieceApplyLeafProc.
    24Jun90  Ted  Renamed PieceApplyLeafProc to PieceApplyProc
    
******************************************************************************/

#import PACKAGE_SPECS
#import "bintreetypes.h"

void PieceAdjust(Piece p, short dx, short dy)
{
    if (p.any->type == DIVPIECE)
	DPAdjust(p.dp, dx, dy);
    else
	BPAdjust(p.bp, dx, dy);
}

void PieceApplyBoundsProc(Piece p, Bounds *bounds, void *data, void(*proc)())
{
    if (p.any->type == DIVPIECE)
	DPApplyBoundsProc(p, bounds, data, proc);
    else
	BPApplyBoundsProc(p, bounds, data, proc);
}

void PieceApplyProc(Piece p, void(*proc)())
{
    if (p.any->type == DIVPIECE) {
	PieceApplyProc(p.dp->l, proc);
	PieceApplyProc(p.dp->g, proc);
    } else (*proc)(p.bp);
}

DivPiece *PieceBecomeDivAt(Piece p, int coord, unsigned char thisOrient)
{
    if (p.any->type == DIVPIECE)
	return DPBecomeDivAt(p.dp, coord, thisOrient);
    else
	return BPBecomeDivAt(p.bp, coord, thisOrient);
}

void PieceCompositeFrom(CompositeOperation *cop)
{
    if (cop->dstD.any->type == DIVPIECE)
	DPCompositeFrom(cop);
    else
	BPCompositeFrom(cop);
}

void PieceCompositeTo(CompositeOperation *cop)
{
    if (cop->srcD.any->type == DIVPIECE)
	DPCompositeTo(cop);
    else
	BPCompositeTo(cop);
}

DivPiece *PieceDivideAt(Piece p, int coord, unsigned char newOrient,
    int causeId)
{
    if (p.any->type == DIVPIECE)
	return DPDivideAt(p.dp, coord, newOrient, causeId);
    else
	return BPDivideAt(p.bp, coord, newOrient, causeId);
}

Bounds *PieceFindPieceBounds(Piece p, Point pt)
{
    if (p.any->type == DIVPIECE)
	return DPFindPieceBounds(p.dp, pt);
    else
	return BPFindPieceBounds(p.bp, pt);
}

void PieceFree(Piece p)
{
    if (p.any->type == DIVPIECE)
	DPFree(p);
    else
	BPFree(p);
}

BitPiece *PieceMark(Piece p, MarkRec *mrec, Bounds *markBds, int instancing,
    int channel)
{
    if (p.any->type == DIVPIECE)
	return DPMark(p.dp, mrec, markBds, instancing, channel);
    else
	return BPMark(p.bp, mrec, markBds, instancing, channel);
}

Piece PieceObscureBecause(Piece p, int cause)
{
    Piece tmp;

    if (p.any->type == DIVPIECE)
	tmp.dp = DPObscureBecause(p.dp, cause);
    else
	tmp.bp = BPObscureBecause(p.bp, cause);
    return tmp;
}

Piece PieceObscureInside(Piece p, Bounds bounds, int cause)
{
    Piece tmp;

    if (p.any->type == DIVPIECE)
	tmp.dp = DPObscureInside(p.dp, bounds, cause);
    else
	tmp = BPObscureInside(p.bp, bounds, cause);
    return tmp;
}

void PiecePrintOn(Piece p, int spaces)
{
    if (p.any->type == DIVPIECE)
	DPPrintOn(p.dp, spaces);
    else
	BPPrintOn(p.bp, spaces);
}

Piece PieceRevealBecause(Piece p, int cause)
{
    Piece tmp;

    if (p.any->type == DIVPIECE)
	tmp.dp = DPRevealBecause(p.dp, cause);
    else
	tmp.bp = BPRevealBecause(p.bp, cause);
    return tmp;
}

Piece PieceRevealInside(Piece p, Bounds bounds, int cause)
{
    Piece tmp;

    if (p.any->type == DIVPIECE)
	tmp = DPRevealInside(p.dp, bounds, cause);
    else
	tmp.bp = BPRevealInside(p.bp, bounds, cause);
    return tmp;
}






