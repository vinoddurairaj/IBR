#define NDRVSYMS 76
#if defined(XCOFF32)
#define NLISTSTRUCT_SIZ sizeof(struct nlist)
#else /* defined(XCOFF32) */
#define NLISTSTRUCT_SIZ sizeof(struct nlist64)
#endif /* defined(XCOFF32) */
