#if !defined(_AIXTIMERS_H_)
#define _AIXTIMERS_H_

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#if defined(_AIX)
#include <sys/timer.h>
#include <sys/m_param.h>
#endif /* defined(_AIX) */

#include "ftd_def.h"

#if defined(_AIX)

/* size of this pool of system trb's */
#define NTRBSLOTS 256

/* XXX TEMP */
#define DBG_TIMERTAB

/* state of a table slot */
enum slotstat_e
  {
    TRBSLOT_FREE = 0,
    TRBSLOT_ALLOC = 1
  };
typedef enum slotstat_e slotstat_t;

/* type of a trb table slot */
struct trbblk_s
  {
#if defined(DBG_TIMERTAB)
    struct trbblk_s *trb_nxt;
#endif				/* defined(DBG_TIMERTAB) */
    slotstat_t trb_stat;
    struct trb *trb_trbp;
      ftd_void_t (*trb_func) ();
    caddr_t trb_arg;
  };
typedef struct trbblk_s trbblk_t;
#define TRBTAB_T_SIZ sizeof(trbblk_t)

#define TRBMEMSIZ (NTRBSLOTS * TRBTAB_T_SIZ)

#endif /* defined(_AIX) */

#endif /* !defined(_AIXTIMERS_H_) */
