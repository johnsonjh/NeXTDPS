#NO_APP
.text
	.even
_ADDC:
	link a6,#0
	moveml #0x3c00,sp@-
	movel a6@(8),d5
	movel a6@(12),d3
	movew #61680,d0
	movew #3855,d4
	clrl d2
	movew d0,d2
	movel d5,d1
	andl d2,d1
	lsrl #4,d1
	andl d3,d2
	lsrl #4,d2
	addw d2,d1
	movew d5,d2
	andw d4,d2
	andw d4,d3
	addw d3,d2
	movew d1,d3
	andw d0,d3
	andw d2,d0
	lsrw #4,d0
	orw d0,d3
	clrl d5
	movew d3,d5
	movel d5,d0
	lsll #1,d0
	orl d0,d5
	movel d5,d0
	lsll #2,d0
	orl d0,d5
	lslw #4,d1
	orw d2,d1
	orw d5,d1
	clrl d0
	movew d1,d0
	moveml a6@(-16),#0x3c
	unlk a6
	rts
.data
	.even
_which.0:
	.long 3
.text
	.even
.globl _BM34MoveRect
_BM34MoveRect:
	link a6,#0
	moveml #0x3f30,sp@-
	movel a6@(8),a0
	movel a6@(12),a2
	movel a6@(16),a1
	movel a6@(20),d7
	movel a6@(24),d5
	movel a6@(28),d4
	tstl d5
	jeq L74
	tstl d4
	jeq L74
	tstl _which.0
	jeq L6
	asll #1,d7
	movel a2,d6
	asll #1,d6
	movel d6,a2
	tstl a6@(32)
	jeq L6
	movel a0,d0
	moveq #3,d6
	andl d6,d0
	seq d1
	moveq #1,d6
	andl d6,d1
	movel a1,d0
	moveq #3,d6
	andl d6,d0
	seq d0
	moveq #1,d6
	andl d6,d0
	cmpl d1,d0
	jeq L8
	moveq #2,d6
	cmpl _which.0,d6
	jne L7
L8:
	movel a0,d0
	moveq #3,d6
	andl d6,d0
	jeq L72
	subql #1,d5
L72:
	movel d5,d2
	asrl #1,d2
	moveq #31,d1
	andl d2,d1
	jra L11
L54:
	movel a0,d0
	moveq #3,d6
	andl d6,d0
	jeq L14
	movew a0@+,a1@+
L14:
	movel d2,d0
	asrl #5,d0
	moveq #31,d6
	cmpl d6,d1
	jhi L15
LI51:
	movew pc@(L51-LI51-2:b,d1:l:2),d6
	jmp pc@(2,d6:w)
L51:
	.word L17-L51
	.word L49-L51
	.word L48-L51
	.word L47-L51
	.word L46-L51
	.word L45-L51
	.word L44-L51
	.word L43-L51
	.word L42-L51
	.word L41-L51
	.word L40-L51
	.word L39-L51
	.word L38-L51
	.word L37-L51
	.word L36-L51
	.word L35-L51
	.word L34-L51
	.word L33-L51
	.word L32-L51
	.word L31-L51
	.word L30-L51
	.word L29-L51
	.word L28-L51
	.word L27-L51
	.word L26-L51
	.word L25-L51
	.word L24-L51
	.word L23-L51
	.word L22-L51
	.word L21-L51
	.word L20-L51
	.word L19-L51
L50:
	movel a0@+,a1@+
L19:
	movel a0@+,a1@+
L20:
	movel a0@+,a1@+
L21:
	movel a0@+,a1@+
L22:
	movel a0@+,a1@+
L23:
	movel a0@+,a1@+
L24:
	movel a0@+,a1@+
L25:
	movel a0@+,a1@+
L26:
	movel a0@+,a1@+
L27:
	movel a0@+,a1@+
L28:
	movel a0@+,a1@+
L29:
	movel a0@+,a1@+
L30:
	movel a0@+,a1@+
L31:
	movel a0@+,a1@+
L32:
	movel a0@+,a1@+
L33:
	movel a0@+,a1@+
L34:
	movel a0@+,a1@+
L35:
	movel a0@+,a1@+
L36:
	movel a0@+,a1@+
L37:
	movel a0@+,a1@+
L38:
	movel a0@+,a1@+
L39:
	movel a0@+,a1@+
L40:
	movel a0@+,a1@+
L41:
	movel a0@+,a1@+
L42:
	movel a0@+,a1@+
L43:
	movel a0@+,a1@+
L44:
	movel a0@+,a1@+
L45:
	movel a0@+,a1@+
L46:
	movel a0@+,a1@+
L47:
	movel a0@+,a1@+
L48:
	movel a0@+,a1@+
L49:
	movel a0@+,a1@+
L17:
	dbra d0,L50
L15:
	btst #0,d5
	jeq L13
	movew a0@+,a1@+
L13:
	addl d7,a1
	addl a2,a0
L11:
	subql #1,d4
	jpl L54
L74:
	moveq #1,d0
	jra L2
L7:
	moveq #3,d6
	cmpl _which.0,d6
	jne L6
	movel a1,d0
	andl d6,d0
	jeq L73
	subql #1,d5
L73:
	movel d5,d6
	asrl #1,d6
	movel d6,a3
	clrl d3
	jra L59
L70:
	movel a1,d0
	moveq #3,d6
	andl d6,d0
	jne L62
	movew a0@+,d3
	movel d3,d1
#APP
	swap d1
#NO_APP
	jra L63
L62:
	movel a0@+,d1
#APP
	swap d1
#NO_APP
	movew d1,a1@+
L63:
	movew a3,d2
	jra L64
L67:
	movel a0@+,d0
#APP
	swap d0
	movew d0,d1
	movel d1, a1@+
#NO_APP
	movel d0,d1
L64:
	dbra d2,L67
	btst #0,d5
	jeq L68
	movel d1,d0
	moveq #16,d6
	lsrl d6,d0
	movew d0,a1@+
	jra L61
L68:
	subqw #2,a0
L61:
	addl d7,a1
	addl a2,a0
L59:
	subql #1,d4
	jpl L70
	jra L74
L6:
	clrl d0
L2:
	moveml a6@(-32),#0xcfc
	unlk a6
	rts
	.even
.globl _BMComposite34
_BMComposite34:
	link a6,#-28
	moveml #0x3f3c,sp@-
	movel a6@(8),a4
	movel a4@(12),d0
	moveq #1,d6
	cmpl d0,d6
	jne L76
	movew a4@(30),d5
	jra L77
L76:
	movel a4@(16),a3
	movel a4@(24),a6@(-8)
	movel a4@(20),a6@(-12)
L77:
	movel a4@(8),d7
	movel a4@(44),a6@(-4)
	movel a4@(36),a2
	movel a4@(40),a6@(-16)
	movel a4@(4),a5
	movew a4@(58),d2
	movel a4@,d1
	moveq #14,d6
	cmpl d6,d1
	jhi L381
LI382:
	movew pc@(L382-LI382-2:b,d1:l:2),d6
	jmp pc@(2,d6:w)
L382:
	.word L79-L382
	.word L80-L382
	.word L140-L382
	.word L159-L382
	.word L178-L382
	.word L197-L382
	.word L216-L382
	.word L235-L382
	.word L254-L382
	.word L273-L382
	.word L292-L382
	.word L311-L382
	.word L330-L382
	.word L362-L382
	.word L343-L382
L79:
	clrw d5
	moveq #1,d0
L80:
	tstl d0
	jne L81
	tstw d2
	jeq L82
	tstl a4@(64)
	jeq L83
	tstl d7
	jle L75
	movew d2,d3
	notw d3
L91:
	movel a5,d4
	jle L443
L90:
	movew d2,d0
	andw a3@+,d0
	movew d3,d1
	andw a2@,d1
	orw d1,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L90
L443:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L91
	jra L75
L83:
	tstl d7
	jle L75
	movew d2,d3
	notw d3
L100:
	movel a5,d4
	jle L441
L99:
	movew d2,d0
	andw a3@,d0
	movew d3,d1
	andw a2@,d1
	orw d1,d0
	movew d0,a2@
	subqw #2,a3
	subqw #2,a2
	subql #1,d4
	tstl d4
	jgt L99
L441:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L100
	jra L75
L82:
	movel a4@(64),sp@-
	movel d7,sp@-
	movel a5,sp@-
	movel a6@(-4),sp@-
	movel a2,sp@-
	movel a6@(-8),sp@-
	movel a3,sp@-
	jbsr _BM34MoveRect
	tstl d0
	jne L75
	tstl a4@(64)
	jeq L103
	tstl d7
	jle L75
	movel a6@(-4),d2
	asll #1,d2
	movel a6@(-8),d1
	asll #1,d1
L111:
	movel a5,d4
	jle L439
L110:
	movew a3@+,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L110
L439:
	subql #1,d7
	addl d2,a2
	addl d1,a3
	tstl d7
	jgt L111
	jra L75
L103:
	tstl d7
	jle L75
	movel a6@(-4),d2
	asll #1,d2
	movel a6@(-8),d1
	asll #1,d1
L120:
	movel a5,d4
	jle L437
L119:
	movew a3@,d0
	movew d0,a2@
	subqw #2,a3
	subqw #2,a2
	subql #1,d4
	tstl d4
	jgt L119
L437:
	subql #1,d7
	addl d2,a2
	addl d1,a3
	tstl d7
	jgt L120
	jra L75
L81:
	tstw d2
	jeq L122
	andw d2,d5
	tstl d7
	jle L75
	movew d2,d1
	notw d1
L130:
	movel a5,d4
	jle L435
L129:
	movew d1,d0
	andw a2@,d0
	orw d5,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L129
L435:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L130
	jra L75
L122:
	tstl d7
	jle L75
	movel a6@(-4),d0
	asll #1,d0
L139:
	movel a5,d4
	jle L433
L138:
	movew d5,a2@+
	subql #1,d4
	tstl d4
	jgt L138
L433:
	subql #1,d7
	addl d0,a2
	tstl d7
	jgt L139
	jra L75
L140:
	tstl d0
	jne L141
	tstl d7
	jle L75
L149:
	movel a5,d4
	jle L431
	clrl d3
L148:
	movew a3@,d5
	movew d5,d3
	movel d3,d0
	notl d0
	moveq #15,d6
	andl d0,d6
	movel d6,a6@(-26)
	movew a2@,d2
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	movew d0,d3
	movel d3,d0
	mulsl d6,d0
	addw #3855,d0
	andw #61680,d0
	movew d2,d1
	andw #3855,d1
	movew d1,d3
	movel d3,d1
	mulsl d6,d1
	addl #3855,d1
	lsrl #4,d1
	andw #3855,d1
	orw d1,d0
	addw d5,d0
	movew d0,a2@
	subql #1,d4
	movel a6@(-16),d6
	lea a2@(d6:l:2),a2
	movel a6@(-12),d6
	lea a3@(d6:l:2),a3
	tstl d4
	jgt L148
L431:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L149
	jra L75
L141:
	clrl a6@(-26)
	movew d5,a6@(-24)
	notl a6@(-26)
	moveq #15,d6
	andl d6,a6@(-26)
	tstl d7
	jle L75
L158:
	movel a5,d4
	jle L429
	clrl d3
L157:
	movew a2@,d2
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	movew d0,d3
	movel d3,d0
	mulsl a6@(-26),d0
	addw #3855,d0
	andw #61680,d0
	movew d2,d1
	andw #3855,d1
	movew d1,d3
	movel d3,d1
	mulsl a6@(-26),d1
	addl #3855,d1
	lsrl #4,d1
	andw #3855,d1
	orw d1,d0
	addw d5,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L157
L429:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L158
	jra L75
L159:
	tstl d0
	jne L160
	tstl d7
	jle L75
	movel a6@(-16),d6
	asll #1,d6
	movel d6,a0
	movel a6@(-12),d3
	asll #1,d3
L168:
	movel a5,d4
	jle L427
L167:
	movew a3@,d5
	movew a2@,d2
	andw #15,d2
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	muls d2,d0
	addw #3855,d0
	andw #-3856,d0
	movew d5,d1
	andw #3855,d1
	mulu d2,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d0,a2@
	subql #1,d4
	addl a0,a2
	addl d3,a3
	tstl d4
	jgt L167
L427:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L168
	jra L75
L160:
	tstl d7
	jle L75
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	movew d0,a6@(-26)
	movew d5,d3
	andw #3855,d3
L177:
	movel a5,d4
	jle L425
L176:
	movew a2@,d2
	andw #15,d2
	movew a6@(-26),d0
	muls d2,d0
	addw #3855,d0
	andw #-3856,d0
	movew d3,d1
	mulu d2,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L176
L425:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L177
	jra L75
L178:
	tstl d0
	jne L179
	tstl d7
	jle L75
	movel a6@(-16),d6
	asll #1,d6
	movel d6,a0
	movel a6@(-12),d3
	asll #1,d3
L187:
	movel a5,d4
	jle L423
L186:
	movew a3@,d5
	movew a2@,d0
	notw d0
	movew d0,d2
	andw #15,d2
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	muls d2,d0
	addw #3855,d0
	andw #-3856,d0
	movew d5,d1
	andw #3855,d1
	mulu d2,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d0,a2@
	subql #1,d4
	addl a0,a2
	addl d3,a3
	tstl d4
	jgt L186
L423:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L187
	jra L75
L179:
	tstl d7
	jle L75
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	movew d0,a6@(-26)
	movew d5,d3
	andw #3855,d3
L196:
	movel a5,d4
	jle L421
L195:
	movew a2@,d0
	notw d0
	movew d0,d2
	andw #15,d2
	movew a6@(-26),d0
	muls d2,d0
	addw #3855,d0
	andw #-3856,d0
	movew d3,d1
	mulu d2,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L195
L421:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L196
	jra L75
L197:
	tstl d0
	jne L198
	tstl d7
	jle L75
L206:
	movel a5,d4
	jle L419
L205:
	movew a3@,d5
	movew d5,d0
	notw d0
	andw #15,d0
	movew d0,a6@(-26)
	movew a2@,d2
	movew d2,d3
	andw #15,d3
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d5,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d2,d1
	andw #61680,d1
	lsrw #4,d1
	muls a6@(-26),d1
	addw #3855,d1
	andw #-3856,d1
	andw #3855,d2
	mulu a6@(-26),d2
	addl #3855,d2
	asrl #4,d2
	andw #3855,d2
	orw d2,d1
	addw d1,d0
	movew d0,a2@
	subql #1,d4
	movel a6@(-16),d6
	lea a2@(d6:l:2),a2
	movel a6@(-12),d6
	lea a3@(d6:l:2),a3
	tstl d4
	jgt L205
L419:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L206
	jra L75
L198:
	movew d5,d6
	notw d6
	movew d6,a6@(-26)
	andw #15,a6@(-26)
	tstl d7
	jle L75
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	movew d0,a6@(-18)
L215:
	movel a5,d4
	jle L417
L214:
	movew a2@,d2
	movew d2,d3
	andw #15,d3
	movew a6@(-18),d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d5,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d2,d1
	andw #61680,d1
	lsrw #4,d1
	muls a6@(-26),d1
	addw #3855,d1
	andw #-3856,d1
	andw #3855,d2
	mulu a6@(-26),d2
	addl #3855,d2
	asrl #4,d2
	andw #3855,d2
	orw d2,d1
	addw d1,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L214
L417:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L215
	jra L75
L216:
	tstl d0
	jne L217
	tstl d7
	jle L75
	movel a6@(-16),d6
	asll #1,d6
	movel d6,a0
L225:
	movel a5,d4
	jle L415
L224:
	movew a3@,d5
	movew a2@,d2
	movew d2,d0
	notw d0
	movew d0,d3
	andw #15,d3
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d5,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	addw d2,d0
	movew d0,a2@
	subql #1,d4
	addl a0,a2
	movel a6@(-12),d6
	lea a3@(d6:l:2),a3
	tstl d4
	jgt L224
L415:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L225
	jra L75
L217:
	tstl d7
	jle L75
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	movew d0,a6@(-26)
	andw #3855,d5
L234:
	movel a5,d4
	jle L413
L233:
	movew a2@,d2
	movew d2,d0
	notw d0
	movew d0,d3
	andw #15,d3
	movew a6@(-26),d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d5,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	addw d2,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L233
L413:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L234
	jra L75
L235:
	tstl d0
	jne L236
	tstl d7
	jle L75
	movel a6@(-16),d6
	asll #1,d6
	movel d6,a0
	movel a6@(-12),d5
	asll #1,d5
L244:
	movel a5,d4
	jle L411
L243:
	movew a3@,d3
	andw #15,d3
	movew a2@,d2
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d2,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d0,a2@
	subql #1,d4
	addl a0,a2
	addl d5,a3
	tstl d4
	jgt L243
L411:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L244
	jra L75
L236:
	movew d5,d3
	andw #15,d3
	tstl d7
	jle L75
	movel a6@(-4),d5
	asll #1,d5
L253:
	movel a5,d4
	jle L409
L252:
	movew a2@,d2
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d2,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L252
L409:
	subql #1,d7
	addl d5,a2
	tstl d7
	jgt L253
	jra L75
L254:
	tstl d0
	jne L255
	tstl d7
	jle L75
	movel a6@(-16),d6
	asll #1,d6
	movel d6,a0
	movel a6@(-12),d5
	asll #1,d5
L263:
	movel a5,d4
	jle L407
L262:
	movew a3@,d0
	notw d0
	movew d0,d3
	andw #15,d3
	movew a2@,d2
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d2,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d0,a2@
	subql #1,d4
	addl a0,a2
	addl d5,a3
	tstl d4
	jgt L262
L407:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L263
	jra L75
L255:
	movew d5,d3
	notw d3
	andw #15,d3
	tstl d7
	jle L75
	movel a6@(-4),d5
	asll #1,d5
L272:
	movel a5,d4
	jle L405
L271:
	movew a2@,d2
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d2,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L271
L405:
	subql #1,d7
	addl d5,a2
	tstl d7
	jgt L272
	jra L75
L273:
	tstl d0
	jne L274
	tstl d7
	jle L75
L282:
	movel a5,d4
	jle L403
L281:
	movew a3@,d5
	movew d5,d6
	andw #15,d6
	movew d6,a6@(-26)
	movew a2@,d2
	movew d2,d0
	notw d0
	movew d0,d3
	andw #15,d3
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	muls d6,d0
	addw #3855,d0
	andw #-3856,d0
	movew d2,d1
	andw #3855,d1
	mulu d6,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d5,d1
	andw #61680,d1
	lsrw #4,d1
	muls d3,d1
	addw #3855,d1
	andw #-3856,d1
	movew d5,d2
	andw #3855,d2
	mulu d3,d2
	addl #3855,d2
	asrl #4,d2
	andw #3855,d2
	orw d2,d1
	addw d1,d0
	movew d0,a2@
	subql #1,d4
	movel a6@(-16),d6
	lea a2@(d6:l:2),a2
	movel a6@(-12),d6
	lea a3@(d6:l:2),a3
	tstl d4
	jgt L281
L403:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L282
	jra L75
L274:
	movew d5,d6
	andw #15,d6
	movew d6,a6@(-26)
	tstl d7
	jle L75
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	movew d0,a6@(-20)
L291:
	movel a5,d4
	jle L401
L290:
	movew a2@,d2
	movew d2,d0
	notw d0
	movew d0,d3
	andw #15,d3
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	muls a6@(-26),d0
	addw #3855,d0
	andw #-3856,d0
	movew d2,d1
	andw #3855,d1
	mulu a6@(-26),d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew a6@(-20),d2
	muls d3,d2
	addw #3855,d2
	andw #-3856,d2
	movew d5,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d2
	addw d2,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L290
L401:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L291
	jra L75
L292:
	tstl d0
	jne L293
	tstl d7
	jle L75
L301:
	movel a5,d4
	jle L399
L300:
	movew a3@,d5
	movew d5,d0
	notw d0
	andw #15,d0
	movew d0,a6@(-26)
	movew a2@,d2
	movew d2,d0
	notw d0
	movew d0,d3
	andw #15,d3
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	muls a6@(-26),d0
	addw #3855,d0
	andw #-3856,d0
	movew d2,d1
	andw #3855,d1
	mulu a6@(-26),d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d5,d1
	andw #61680,d1
	lsrw #4,d1
	muls d3,d1
	addw #3855,d1
	andw #-3856,d1
	movew d5,d2
	andw #3855,d2
	mulu d3,d2
	addl #3855,d2
	asrl #4,d2
	andw #3855,d2
	orw d2,d1
	addw d1,d0
	movew d0,a2@
	subql #1,d4
	movel a6@(-16),d6
	lea a2@(d6:l:2),a2
	movel a6@(-12),d6
	lea a3@(d6:l:2),a3
	tstl d4
	jgt L300
L399:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L301
	jra L75
L293:
	movew d5,d6
	notw d6
	movew d6,a6@(-26)
	andw #15,a6@(-26)
	tstl d7
	jle L75
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	movew d0,a6@(-22)
L310:
	movel a5,d4
	jle L397
L309:
	movew a2@,d2
	movew d2,d0
	notw d0
	movew d0,d3
	andw #15,d3
	movew d2,d0
	andw #61680,d0
	lsrw #4,d0
	muls a6@(-26),d0
	addw #3855,d0
	andw #-3856,d0
	movew d2,d1
	andw #3855,d1
	mulu a6@(-26),d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew a6@(-22),d2
	muls d3,d2
	addw #3855,d2
	andw #-3856,d2
	movew d5,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d2
	addw d2,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L309
L397:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L310
	jra L75
L311:
	tstl d0
	jne L312
	tstl d7
	jle L75
L320:
	movel a5,d4
	jle L395
	clrl d3
L319:
	movew a3@,d5
	movew d5,d0
	notw d0
	andw #-16,d0
	movew d5,d1
	andw #15,d1
	movew d0,d5
	orw d1,d5
	movew a2@,d2
	movew d2,d0
	notw d0
	andw #-16,d0
	movew d2,d1
	andw #15,d1
	movew d0,d2
	orw d1,d2
	movew d2,d3
	movel d3,sp@-
	movew d5,d3
	movel d3,sp@-
	jbsr _ADDC
	movew d0,d2
	notw d0
	andw #-16,d0
	movew d2,d1
	andw #15,d1
	orw d1,d0
	movew d0,a2@
	addqw #8,sp
	subql #1,d4
	movel a6@(-16),d6
	lea a2@(d6:l:2),a2
	movel a6@(-12),d6
	lea a3@(d6:l:2),a3
	tstl d4
	jgt L319
L395:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L320
	jra L75
L312:
	movew d5,d0
	notw d0
	andw #-16,d0
	movew d5,d1
	andw #15,d1
	movew d0,d5
	orw d1,d5
	tstl d7
	jle L75
L329:
	movel a5,d4
	jle L393
	clrl d3
L328:
	movew a2@,d2
	movew d2,d0
	notw d0
	andw #-16,d0
	movew d2,d1
	andw #15,d1
	movew d0,d2
	orw d1,d2
	movew d2,d3
	movel d3,sp@-
	movew d5,d3
	movel d3,sp@-
	jbsr _ADDC
	movew d0,d2
	notw d0
	andw #-16,d0
	movew d2,d1
	andw #15,d1
	orw d1,d0
	movew d0,a2@+
	addqw #8,sp
	subql #1,d4
	tstl d4
	jgt L328
L393:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L329
	jra L75
L330:
	tstl d7
	jle L75
	movel a6@(-4),d0
	asll #1,d0
L342:
	movel a5,d4
	jle L391
L341:
	movew a2@,d2
	andw #65520,d2
	cmpw #65520,d2
	jne L337
	movew #43680,a2@+
	jra L336
L337:
	cmpw #43680,d2
	jne L339
	movew #65520,a2@+
	jra L336
L339:
	addqw #2,a2
L336:
	subql #1,d4
	tstl d4
	jgt L341
L391:
	subql #1,d7
	addl d0,a2
	tstl d7
	jgt L342
	jra L75
L343:
	movew a4@(54),d3
	andw #15,d3
	movew d3,d6
	notw d6
	movew d6,a6@(-26)
	andw #15,a6@(-26)
	tstl a4@(64)
	jeq L344
	tstl d7
	jle L75
L352:
	movel a5,d4
	jle L389
L351:
	movew a3@+,d5
	movew a2@,d2
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d5,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d2,d1
	andw #61680,d1
	lsrw #4,d1
	muls a6@(-26),d1
	addw #3855,d1
	andw #-3856,d1
	andw #3855,d2
	mulu a6@(-26),d2
	addl #3855,d2
	asrl #4,d2
	andw #3855,d2
	orw d2,d1
	addw d1,d0
	movew d0,a2@+
	subql #1,d4
	tstl d4
	jgt L351
L389:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L352
	jra L75
L344:
	tstl d7
	jle L75
L361:
	movel a5,d4
	jle L387
L360:
	movew a3@,d5
	subqw #2,a3
	movew a2@,d2
	movew d5,d0
	andw #61680,d0
	lsrw #4,d0
	muls d3,d0
	addw #3855,d0
	andw #-3856,d0
	movew d5,d1
	andw #3855,d1
	mulu d3,d1
	addl #3855,d1
	asrl #4,d1
	andw #3855,d1
	orw d1,d0
	movew d2,d1
	andw #61680,d1
	lsrw #4,d1
	muls a6@(-26),d1
	addw #3855,d1
	andw #-3856,d1
	andw #3855,d2
	mulu a6@(-26),d2
	addl #3855,d2
	asrl #4,d2
	andw #3855,d2
	orw d2,d1
	addw d1,d0
	movew d0,a2@
	subqw #2,a2
	subql #1,d4
	tstl d4
	jgt L360
L387:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L361
	jra L75
L362:
	tstl d0
	jne L363
	tstl d7
	jle L75
L371:
	movel a5,d4
	jle L385
	clrl d3
L370:
	movew a3@,d5
	movew a2@,d2
	movew d2,d3
	movel d3,sp@-
	movew d5,d3
	movel d3,sp@-
	jbsr _ADDC
	movew d0,a2@
	addqw #8,sp
	subql #1,d4
	movel a6@(-16),d6
	lea a2@(d6:l:2),a2
	movel a6@(-12),d6
	lea a3@(d6:l:2),a3
	tstl d4
	jgt L370
L385:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	movel a6@(-8),d6
	lea a3@(d6:l:2),a3
	tstl d7
	jgt L371
	jra L75
L363:
	tstl d7
	jle L75
L380:
	movel a5,d4
	jle L383
	clrl d3
L379:
	movew a2@,d2
	movew d2,d3
	movel d3,sp@-
	movew d5,d3
	movel d3,sp@-
	jbsr _ADDC
	movew d0,a2@+
	addqw #8,sp
	subql #1,d4
	tstl d4
	jgt L379
L383:
	subql #1,d7
	movel a6@(-4),d6
	lea a2@(d6:l:2),a2
	tstl d7
	jgt L380
	jra L75
L381:
	jbsr _CantHappen
L75:
	moveml a6@(-68),#0x3cfc
	unlk a6
	rts
