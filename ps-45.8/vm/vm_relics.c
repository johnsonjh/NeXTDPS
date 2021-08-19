public integer WordsForChars(nchars)  integer nchars;
{
  return (nchars + (chPerWd - 1)) / chPerWd;
}

#if VMINIT
public AssembleROM(ptr, count)
  charptr ptr;
  integer count;
{
#if CANWRITEOBJ
  register wordptr w = (wordptr) ptr;
  register integer nwords = (count + sizeof (word) - 1) / sizeof (word);

#if	OS !=os_apollo && OS != os_vms
  ObjPutText ((char *) w, nwords * sizeof (word));
#endif	OS !=os_apollo && OS != os_vms
#if OS == os_vms
  register integer i,
          j;
  register boolean done = false;

  for (i = 0; i < nwords; i++) {
    j = i;
    while (w[i] == 0) {
      if (++i >= nwords) {
	done = true;
	break;
      }
    }
    if (i > j) {
      j = (i - j) * sizeof (word);
      OutputBlock (&j);
    }
    if (done)
      return;
    AddBuffer (&w[i]);
  }
#endif OS == os_vms
#else CANWRITEOBJ
CantHappen();
#endif CANWRITEOBJ
}				/* end of AssembleROM */
#endif	VMINIT

#if VMINIT && false
public integer CompressVM(scratch)
  charptr scratch;
{
  integer zc,
          i;
  charptr source = (charptr) vmRAMOffset;
  charptr end = (charptr) vmPrivate->free;
  charptr countbyte,
          scrstart = scratch;

  countbyte = scratch;
  while (true) {
start:
    Assert ((countbyte == scratch) || (*countbyte != 0));
    countbyte = scratch++;
    if (source == end)
      break;
zero:
    if ((*source == 0) && (*(source + 1) == 0)) {
      zc = 0;
      while (zc < 255) {
	if (source == end)
	  break;
	if (*source++ != 0) {
	  source--;
	  break;
	}
	zc++;
      }
      if (zc < 15)
	*countbyte += zc;
      else {
	*countbyte += 15;
	*scratch++ = zc;
      }
      goto start;
    } else {
      if (*countbyte != 0)
	goto start;
    }
    for (i = 0; i < 15; i++) {
      if (source == end) {
	Assert (i != 0);
	*countbyte = i << 4;
	goto start;
      }
      if ((*source == 0) && (*(source + 1) == 0)) {
	Assert (i != 0);
	*countbyte = i << 4;
	goto zero;
      }
      *scratch++ = *source++;
    }
    *countbyte = 15 << 4;
    goto zero;
  }
  *countbyte++ = 0;
  return ((integer) countbyte - (integer) scrstart);
}				/* end of CompressVM */
#endif VMINIT

#if	false
DecompressVM ()
  /*
   * Expand a RAM image into RAM segment.
   */
{
#ifdef VAXC
  globalref vmROM,
	    vmRAM,
            vmRAMinit,
            vmRAMisize;

#else VAXC
  extern    vmROM,
            vmRAM,
            vmRAMinit,
            vmRAMisize;
#endif VAXC

  vmBase = (charptr) & vmROM;

#if VMABSOLUTE
  vm = 0;
  vmBaseOffset = (Offset) vmBase;
#else VMABSOLUTE
  vm = vmBase;
  vmBaseOffset = (Offset) 0;
#endif VMABSOLUTE

  vmShared = (VMInfo *) & vmROM;
  vmRAMOffset = (charptr) & vmRAM - vm;
  vmDELTA = vmRAMOffset - vmBaseOffset;

  BLTvmRAM (&vmRAMinit, &vmRAM, vmRAMisize);

  vmInfo = vmPrivate = (VMInfo *) & vmRAM;
}
#endif	false

#if false
private BLTvmRAM(from, to, size)
  register charptr	from,
  			to;
  integer		size;
  /*
   * Fast transfer of non-zero blocks from address "from" to address "to"
   */
{
  register integer zc	= 0,
          	   nzc	= 0;

  os_bzero ((char *) to, (int) size);

#if MC68K
  asm ("BLT00:");
  asm ("	moveq	#0,d7");
  asm ("	movb	a5@+,d7");
  asm ("	tstw	d7");
  asm ("	jeq	BLT99");
  asm ("	movl	d7,d6");
  asm ("	andb	#15,d7");
  asm ("	lsrb	#4,d6");
  asm ("	tstw	d6");
  asm ("	jeq	BLT20");
  asm ("BLT10:");
  asm ("	subqw	#1,d6");
  asm ("BLT11:");
  asm ("	movb	a5@+,a4@+");
  asm ("	dbra	d6,BLT11");
  asm ("BLT20:");
  asm ("	tstw	d7");
  asm ("	jeq	BLT00");
  asm ("	cmpw	#15,d7");
  asm ("	jne	BLT21");
  asm ("	movb	a5@+,d7");
  asm ("BLT21:");
  asm ("	addl	d7,a4");
  asm ("	jra	BLT00");
  asm ("BLT99:");
#else MC68K
  while (true) {
    zc = (*from++);

    if (zc == 0)
      break;

    nzc = zc;
    zc = zc & 15;
    nzc = nzc >> 4;

#if SPACE
    NZchunks[nzc]++;
#endif SPACE

    while (--nzc >= 0) {
      *to++ = *from++;
    }

    if (zc == 15) {
      zc = *from++;
    }

#if SPACE
    Zchunks[zc]++;
#endif SPACE

    to += zc;
  }
#endif MC68K
}				/* end of BLTvmRAM */
#endif false

