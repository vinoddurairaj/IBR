/* stolen from Sun sysmacros.h */

#define	O_BITSMAJOR	7	/* # of SVR3 major device bits */
#define	O_BITSMINOR	8	/* # of SVR3 minor device bits */
#define	O_MAXMAJ	0x7f	/* SVR3 max major value */
#define	O_MAXMIN	0xff	/* SVR3 max major value */


#define	L_BITSMAJOR	14	/* # of SVR4 major device bits */
#define	L_BITSMINOR	18	/* # of SVR4 minor device bits */
#define	L_MAXMAJ	0x3fff	/* SVR4 max major value */
#define	L_MAXMIN	0x3ffff	/* MAX minor for 3b2 software drivers. */
				/* For 3b2 hardware devices the minor is */
				/* restricted to 256 (0-255) */

#ifdef KERNEL

/* major part of a device internal to the kernel */

#define	major(x)	(int)((unsigned)((x)>>O_BITSMINOR) & O_MAXMAJ)
#define	bmajor(x)	(int)((unsigned)((x)>>O_BITSMINOR) & O_MAXMAJ)

/* get internal major part of expanded device number */

#define	getmajor(x)	(int)((unsigned)((x)>>L_BITSMINOR) & L_MAXMAJ)

/* minor part of a device internal to the kernel */

#define	minor(x)	(int)((x) & O_MAXMIN)

/* get internal minor part of expanded device number */

#define	getminor(x)	(int)((x) & L_MAXMIN)

#else

/* major part of a device external from the kernel (same as emajor below) */

#define	major(x)	(int)(((unsigned)(x) >> O_BITSMINOR) & O_MAXMAJ)


/* minor part of a device external from the kernel  (same as eminor below) */

#define	minor(x)	(int)((x) & O_MAXMIN)

#endif	/* _KERNEL */

/* create old device number */

#define	makedev(x, y)	(unsigned short)(((x)<<O_BITSMINOR) | ((y)&O_MAXMIN))

/* make an new device number */

#define	makedevice(x, y) (unsigned long)(((x)<<L_BITSMINOR) | ((y)&L_MAXMIN))


/*
 *   emajor() allows kernel/driver code to print external major numbers
 *   eminor() allows kernel/driver code to print external minor numbers
 */

#define	emajor(x) \
	(int)(((unsigned long)(x)>>O_BITSMINOR) > O_MAXMAJ) ? \
	    NODEV : (((unsigned long)(x)>>O_BITSMINOR)&O_MAXMAJ)

#define	eminor(x) \
	(int)((x)&O_MAXMIN)

/*
 * get external major and minor device
 * components from expanded device number
 */
#define	getemajor(x)	(int)((((unsigned long)(x)>>L_BITSMINOR) > L_MAXMAJ) ? \
			NODEV : (((unsigned long)(x)>>L_BITSMINOR)&L_MAXMAJ))
#define	geteminor(x)	(int)((x)&L_MAXMIN)
