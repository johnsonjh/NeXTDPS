#define HMASK 0xaaaaaaaa /* 10101010101010101010101010101010 */

#define WRITE(wf,s,dp) do { uint d;			\
  d = *dp; wf(s,d); *dp = d;				\
} while(0)

#define WRITEMASK(wf,s,dp,mask) do { uint d,od,msk; 	\
  od = d = *dp; wf(s,d);				\
  msk = mask;						\
  *dp = (d&msk) | (od&~msk);				\
} while(0)

#define DoWF0(s,d) do { uint m;				\
  m = (s^(s+s))&(d^(d+d))&HMASK;			\
  d &= s;						\
  if(m) { d ^= m>>1; d ^= m&d; }			\
} while(0)

#define DoWF1(s,d) do { uint b,m;			\
  b = ~(s^d);						\
  d |= s;						\
  if(m = (b&(b+b))&(s^(s+s))&HMASK) d = (d^(m>>1))|m;	\
} while(0)

#define DoWF2(s,d) do { uint m;				\
  m = (s^(s+s))&(d^(d+d))&HMASK;			\
  d &= ~s;						\
  if(m) { d ^= m>>1; d ^= m&d; }			\
} while(0)

#define DoWF3(s,d) do { uint m;				\
  m = (s^(s+s))&(d^(d+d))&HMASK;			\
  d |= s;						\
  if(m) { d ^= m>>1; d ^= m&~d; }			\
} while(0)

#define DoWF4(s,d) do { uint m;				\
  m = (s^(s+s))&(d^(d+d))&HMASK;			\
  d &= s;						\
  if(m) { d &= ~(m>>1); m &= d; d ^= m|(m>>1); }	\
} while(0)



