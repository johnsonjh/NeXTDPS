	.native
	.text
	.globl _fixmul
_fixmul:
			mpy .r0,.r1		# 32 x 32 multiply
			nop			# wait 5 more cycles
			movi 0x8000,.r4		# for rounding result
			movih 0,.r4
			nop
			nop
			mov .am,.r2		# high bits in r2
	br .true LL10;	addi 0,.r2,.r2,.gez	# jump if high bits positive
	ovneut;		mov .al,.r3		# low result bits in r3
			subi .r4,1,.r4		# round away from zero for neg.
LL10:	br .true LL30;	add .r4,.r3,.r3,.nov	# round the low order bits
	br .true MDOv;	addi 1,.r2,.r2,.ovf	# propagate overflow
LL30:	br .true LL35;	ext .r2,15,0,.r4,.eqz	# test high 17 bits for overflo
	br .true MDOv;	not .r4,.r4,.nez	# must be all 1's or all 0's
LL35:			ext .r3,16,16,.r0	# high half of r3 is fraction
			mer .r2,16,16,.r0	# low half of r2 is integer
			rts
MDOv:			xor .r0,.r1,.r1		# return 0x7fffffff for neg
			movi 0xffff,.r0		#  overflow, 0x80000000 for
	br .true LL60;	addi 0,.r1,.r1,.ltz	#  positive overflow
	ovneut;		movih 0x7fff,.r0
			addi 1,.r0,.r0
LL60:			rts


	.globl	_fracmul
	.globl	_fxfrmul
_fracmul:
_fxfrmul:
			mpy .r0,.r1		# 32 x 32 multiply
			nop			# wait 5 more cycles
			movi 0,.r4		# for rounding result
			movih 0x2000,.r4
			nop
			nop
			mov .am,.r2		# high bits in r2
	br .true LL110;	addi 0,.r2,.r2,.gez	# jump if high bits positive
	ovneut;		mov .al,.r3		# low result bits in r3
			subi .r4,1,.r4		# round away from zero for neg.
LL110:	br .true LL130;	add .r4,.r3,.r3,.nov	# round the low order bits
	br .true LL132;	addi 1,.r2,.r2,.ovf	# propagate overflow
LL130:	br .true LL135;	ext .r2,29,0,.r4,.eqz	# test high 3 bits for overflow
	br .true LL135;	not .r4,.r4,.eqz	# must be all 1's or all 0's
LL132:	br MDOv;				# jump if overflow
LL135:			ext .r3,30,2,.r0	# high 2 bits of r3 is low rslt
			mer .r2,2,30,.r0	# low 30 bits of r2 is hi rslt
			rts

	.globl	_fixdiv
	.globl	_fracratio
_fixdiv:					# compute x/y
_fracratio:
	br .true LL210;	addi 0,.r0,.r2,.gez	# x in r2 and r0; jump if pos.
			neg .r2,.r2		# make x positive
LL210:			ext .r2,15,0,.r4	# high 17 bits into .am
			ldamal .r4,.r4		# needs extra instr. for .am
			dep .r2,17,15,.r2	# low 15 bits into r2 high bits

comdiv:
	br .true LL215;	addi 0,.r1,.r1,.nez	# test divide by zero
	br MDOv;				# jump if zero
LL215:	br .true LL220;	addi 0,.r1,.r3,.gtz	# y in r1 and r3, jump if pos.
			neg .r3,.r3		# make y positive
LL220:			div .r2,.r3		# divide am..r2 by .r3
			nop			# overflow not detected
			nop
			nop
			nop
			nop
			xor .r0,.r1,.r4		# check sign of result
			nop			# while we're waiting
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop			# wait 16 cycles for div
			mov .al,.r2		# .al has quotient
	br .true LL230;	ext .r2,0,1,.r3,.eqz	# jump if round bit clear
	ovneut;		ext .r2,1,31,.r2	# shift left by one
	br .true LL230;	addi 1,.r2,.r2,.nov	# round up; jump if overflow
	br MDOv;				# jump if overflow
LL230:	br .true LL240;	addi 0,.r4,.r4,.gez	# jump if result sign positive
	ovneut;		mov .r2,.r0		# prepare result in r0
			neg .r0,.r0		# negate if result negative
LL240:			rts

	.globl	_fixratio
_fixratio:
	br .true LL310;	addi 0,.r0,.r2,.gez	# x in r2 and r0; jump if pos.
			neg .r2,.r2		# make x positive
LL310:			ext .r2,1,0,.r4		# high 31 bits into .am
			ldamal .r4,.r4		# needs extra instr. for .am
			dep .r2,31,1,.r2	# low 1 bit into r2 high bits
	br comdiv;				# rest same as fixdiv

	.globl	_ufixratio
_ufixratio:
			ext .r0,15,0,.r4	# high 17 bits into .am
			ldamal .r4,.r4		# needs extra instr. for .am
			dep .r0,17,15,.r0	# low 15 bits into r2 high bits
	br .true LL515;	addi 0,.r1,.r1,.nez	# test divide by zero
	br PosErr;				# jump if zero
LL515:			div .r0,.r1		# divide am..r2 by .r3
			nop			# overflow not detected
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop			# wait 16 cycles for div
			mov .al,.r0		# .al has quotient
	br .true LL530;	ext .r0,0,1,.r2,.eqz	# jump if round bit clear
	ovneut;		ext .r0,1,31,.r0	# shift left by one
	br .true PosErr; addi 1,.r0,.r0,.ovf	# round up; jump if overflow
LL530:			rts
PosErr:			movi 0xffff,.r0
			movih 0x7fff,.r0
			rts


	.globl	_fracsqrt
_fracsqrt:
			mov .r0,.r3
			movi 0,.r0
			movi 0,.r1
			movih 0x4000,.r1
			movi 0,.r2
			movi 32,.r4		# loop counter
			pushs .r4
			sub .r3,.r1,.r3		# trial subtraction
frsqrtloop:
	br .true LL410;	subc .r2,.r0,.r2,.ltz	#  of r2,r3 - r0,r1
			add .r0,.r0,.r0		# it worked, r0 = 2*r0 + 1
	br .true LL420;	addi 1,.r0,.r0,.nez	# this jump always works
	ovneut;		add .r3,.r3,.r3		# gain one cycle in the shadow
LL410:			add .r3,.r1,.r3		# add back r0,r1
			addc .r2,.r0,.r2	#   ditto
			add .r0,.r0,.r0		# r0 = 2*r0
			add .r3,.r3,.r3		# shift r2,r3 by 2 to the right
LL420:			addc .r2,.r2,.r2	#   ditto
			add .r3,.r3,.r3		#   ditto
	shsob frsqrtloop; addc .r2,.r2,.r2	#   ditto, repeat 32 times
	revneut;	sub .r3,.r1,.r3		# unrolled trial sub. 1st inst.
			ext .r0,0,1,.r5		# shift right by one
			ext .r0,1,31,.r0	#   and round
			add .r0,.r5,.r0		# round up if low bit was one
			rts

toobig:
	br .true LL710;	addi 0,.r0,.r0,.ltz
			movi 0xffff,.r0
			movih 0x7fff,.r0
			rts
LL710:			movi 0,.r0
			movih 0x8000,.r0
			rts

	.globl _pflttofrac
_pflttofrac:
			addr .r0,0,.word
	load .r0
			ext .r0,23,8,.r1	# get exponent
			movi 128,.r2		# compare against 127+1
	br .true DBints;sub .r1,.r2,.r2,.ltz	# if ge 2^1, we lose
	ovneut;		addi10 30,.r1,.r1	# scale by 2^30 for frac
	br toobig;

	.globl	_pflttofix
_pflttofix:
			addr .r0,0,.word
	load .r0
			ext .r0,23,8,.r1	# get exponent
			movi 142,.r2		# compare against 127+15
	br .true DBints;sub .r1,.r2,.r2,.ltz	# if ge 2^15, we lose
	ovneut;		addi10 16,.r1,.r1	# scale by 2^16 for fixed
	br toobig;

DBints:						# convert float to integer
						# .r1 already has exponent
			movi 127,.r3
	br .true LL610;	sub .r1,.r3,.r1,.gez	# if exp < 0, result = 0
	ovneut;		movi 31,.r3		# prepare for next case
			movi 0,.r0		# return 0
			rts
LL610:	br .true LL620;	sub .r1,.r3,.r3,.ltz	# if exp >= 31, overflow
	ovneut;		movi 0,.r2		# prepare for next case
	br toobig;
LL620:			movih 0x80,.r2		# explicit leading 1
			mer .r0,0,23,.r2	# get fractional part
			movi 23,.r3		# check for right or left shift
	br .true LL630;	sub .r1,.r3,.r1,.gtz	# jump if left shift
			neg .r1,.r1		# prepare right shift amount
			mov .r1,.sar
	br .true LL640;	ext .r2,.sar,0,.r2,.nez	# do the shift
LL630:			mov .r1,.sar		# prepare to shift left
			dep .r2,.sar,0,.r2	# shift left by exponent - 23
LL640:	br .true LL650;	addi 0,.r0,.r0,.gez	# jump if positive
	ovneut;		mov .r2,.r0		# put positive result in r0
			neg .r0,.r0		# negate if negative
LL650:			rts

	.globl	_fixtopflt
_fixtopflt:
	br .true LL810;	addi 0,.r0,.r2,.gez	# check for negative
			neg .r0,.r0
LL810:	br .true LL820;	ffo .r0,.r3,.ne32	# find first one, return if 0
			addr .r1,0,.word
			store .r0
			rts
LL820:			mov .r3,.sar
			dep .r0,.sar,0,.r0	# left justify leading 1
	br .true LL830;	addi10 .r0,0x80,.r0,.nov # round up
			addi -1,.r3,.r3		# adjust shift amount if ovf
LL830:			ext .r0,8,23,.r0	# place mantissa in position
			movi 142,.r4
			sub .r4,.r3,.r4		# calculate exponent
			mer .r4,23,8,.r0	# place exponent in position
	br .true LL840; addi 0,.r2,.r2,.gez	# check for negative again
			movi 1,.r3
			mer .r3,31,1,.r0	# set sign bit
LL840:			addr .r1,0,.word
			store .r0
			rts
