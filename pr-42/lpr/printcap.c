#include <ctype.h>
#include <stdio.h>
#include <syslog.h>
#include <printerdb.h>

static const prdb_ent *pr_ent;


char	*pgetstr();
char	*tdecode();

static char prdb_open = 0;
/*
 * Similar to tgetent except it returns the next enrty instead of
 * doing a lookup.
 */
getprent(bp)
 register char *bp;
{
  
  if( !prdb_open ){
    prdb_set( ".." );
    prdb_open = 1;
  }

  pr_ent = prdb_get();
  if( pr_ent )
    strcpy( bp, *pr_ent->pe_name );
  return( (int)pr_ent );
}

endprent()
{
  prdb_end();
  prdb_open = 0;
}


/*
 * Get an entry for terminal name in buffer bp,
 * from the termcap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
pgetent(bp, name)
	char *bp, *name;
{
	prdb_property *p;
	int i;
	char buf[8192];

/* 
 * Hopefully this fixes it
 * Use the ROOT domain for prdb_set().
 */
	prdb_set( ".." );
#ifdef old_one
	prdb_set( NULL );
#endif old_one

	return( (int)pr_ent = prdb_getbyname (name));
}


/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
pgetnum(id)
	char *id;
{
  register int i, n, base;
  register char *bp;
  prdb_property *p;

  if( pr_ent ){
	  
    for( i=0, p=pr_ent->pe_prop; i<pr_ent->pe_nprops; p=p++, i++){
      if( strcmp( p->pp_key, id ) == 0 ){
	base = 10;
	bp = p->pp_value;
	if( *bp == '#' )
	  *bp++;
	if (*bp == '0')
	  base = 8;
	n = 0;
	while (isdigit(*bp))
	  n *= base, n += *bp++ - '0';
	return( n );
      }
    }
    return( 0 );
  }
  return( 0 );
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
pgetflag( char *id)
{
  register char *bp;
  prdb_property *p;
  int i;

  if( pr_ent ){
    for( i=0, p=pr_ent->pe_prop; i<pr_ent->pe_nprops; p=p++, i++){
      if( strcmp( p->pp_key, id ) == 0 ){
	return( 1 );
      }
    }
  }
  return( 0 );
}


/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
char *
pgetstr(id, area)
	char *id, **area;
{
  register char *bp;
  prdb_property *p;
  int i;

  if( pr_ent ){
    for( i = 0,p = pr_ent->pe_prop; i < pr_ent->pe_nprops; p = p++,i++){
      if( strcmp( p->pp_key, id ) == 0 ){
	return( tdecode( p->pp_value, area ) );
      }
    }
  }
  return( 0 );
}

/*
 * Tdecode does the grung work to decode the
 * string capability escapes.
 */
static char *
tdecode(str, area)
	register char *str;
	char **area;
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = *area;
	while ((c = *str++) && c != ':') {
		switch (c) {

		case '^':
			c = *str++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (isdigit(c)) {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *str++ - '0';
				while (--i && isdigit(*str));
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return (str);
}
