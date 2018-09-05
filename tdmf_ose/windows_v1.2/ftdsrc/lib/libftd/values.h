/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_VALUES_H
#define	_VALUES_H

#pragma ident	"@(#)values.h	1.13	94/07/29 SMI"	/* SVr4.0 1.33	*/

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * These values work with any binary representation of integers
 * where the high-order bit contains the sign.
 */

/* a number used normally for size of a shift */
#define	BITSPERBYTE	8

#define	BITS(type)	(BITSPERBYTE * (int)sizeof (type))

/* short, regular and long ints with only the high-order bit turned on */
#define	HIBITS	((short)(1 << BITS(short) - 1))

#if defined(__STDC__)

#define	HIBITI	(1U << BITS(int) - 1)
#define	HIBITL	(1UL << BITS(long) - 1)

#else

#define	HIBITI	((unsigned)1 << BITS(int) - 1)
#define	HIBITL	(1L << BITS(long) - 1)

#endif

#define	MAXINT	((int)(~HIBITI))

#ifdef	__cplusplus
}
#endif

#endif	/* _VALUES_H */
