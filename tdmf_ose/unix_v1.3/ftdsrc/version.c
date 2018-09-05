#ifdef VERSION
#if defined(_AIX)
volatile char qdsreleasenumber[] = PRODUCTNAME" Release Version " VERSION BLDSEQNUM;
#else /* defined(_AIX) */
static char qdsreleasenumber[] = "Softek " PRODUCTNAME" Release Version " VERSION BLDSEQNUM;
#endif /* defined(_AIX) */
#endif
