#ifdef VERSION
#if defined(_AIX)
volatile char qdsreleasenumber[] = PRODUCTNAME" Release Version " VERSION""VERSIONBUILD;
#else /* defined(_AIX) */
static char qdsreleasenumber[] = PRODUCTNAME" Release Version " VERSION""VERSIONBUILD;
#endif /* defined(_AIX) */
#endif
