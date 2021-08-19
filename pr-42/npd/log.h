/*
 * log.h	Logging routines interface file.
 *
 * Copyright (c) 1990 by NeXT, Inc.,  All rights reserved.
 */

/*
 * External routines.
 */
extern void	LogInit();
extern void	LogWarning(const char *fmt, ...);
extern void	LogError(const char *fmt, ...);
extern void	LogMallocDebug(int mallocError);

/*
 * Conditional debugging routines
 */
#ifdef	DEBUG
extern void	LogDebug(const char *fmt, ...);
#else	DEBUG
extern void	LogDebug(const char *fmt, ...);
#endif	DEBUG
