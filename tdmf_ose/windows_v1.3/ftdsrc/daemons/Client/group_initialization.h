#if defined(_DEBUG)
#include <conio.h>
#endif

#include "ftd_port.h"
#include "ftd_config.h"
#include "ftd_lg.h"
#include "ftdio.h"
#include "ftd_ps.h"
#include "ftd_error.h"
#include "ftd_sock.h"
#include "ftd_stat.h"
#include "ftd_devlock.h"
#include "ftd_mngt.h"

//libftd, ftd_mngt_msgs.cpp
void ftd_mngt_msgs_log(const char **argv, int argc);
static int			autostart;
static int start_group(int group, int force, int autostart);
