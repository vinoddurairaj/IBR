#
# Makefile for Agent
# Warning: Use /usr/local/bin/gmake
#
TOP=..

CDEFINES =
CINCLUDES= ${INCDEVICE} ${INCGEN} ${INCLIC} ${INCMD5}
LIBS= ${LIBGEN} ${LIBLIC} ${SOCKET_LIBS} ${LVM_LIBS}
LDFLAGS=$(UTARGET) ${LINKOPTS}
#
# WRs PROD5602 PROD5610, PROD5628
# For HP-UX >= 11.23, enable long host names API (for agent uname() calls)
# Note: we no longer support HP releases < 11.11, so we check just this level
#
include ${TOP}/mk/vars.gmk
ifeq (HP-UX,$(SYSTYPE))
ifneq ($(SYSVERS),1111)
CDEFINES += -D_HPUX_API_LEVEL=20040821
endif
endif

HDRS =
OBJS =
MAIN_OBJS=iputil.o ftd_mngt.o Agent_uty.o Agent_sock.o Agent_proc.o \
	Agent_msg.o Agent_main.o 
CFG_OBJS=Agent_uty.o Agent_msg.o Agent_config.o iputil.o

PROGRAMS = in.${QAGN} ${Q}agentset launchagent killagent ${PKGNAME}-startagent
BIN_INSTALL=in.${QAGN} ${Q}agentset launchagent killagent
INIT_INSTALL=${PKGNAME}-startagent

EXTRA_CLEANS=${MAIN_OBJS} ${CFG_OBJS} ${PROGRAMS}

include ${TOP}/mk/Makefile.inc
include ${TOP}/mk/debug.gmk

in.${QAGN}: ${MAIN_OBJS}
	$(CC) $(LDFLAGS) -o $@ $(MAIN_OBJS) $(LIBS)

${Q}agentset: ${CFG_OBJS}
	$(CC) $(LDFLAGS) -o $@ $(CFG_OBJS) $(LIBS)


iputil.o:	tdmfAgent.h iputil.c

ftd_mngt.o:	tdmfAgent.h tdmfAgent_trace.h \
	ftd_mngt.c \
	ftd_mngt_gather_server_info.c \
	ftd_mngt_StatusMsgThread.c \
	ftd_mngt_performance_send_data.c \
	ftd_mngt_send_lg_state.c \
	ftd_mngt_sys_request_system_restart.c

Agent_uty.o:	tdmfAgent.h tdmfAgent_trace.h \
	Agent_uty.c

Agent_sock.o:	tdmfAgent.h tdmfAgent_trace.h \
	Agent_sock.c

Agent_proc.o:	tdmfAgent.h tdmfAgent_trace.h \
	Agent_proc.c \
	ftd_mngt_set_config.c \
	ftd_mngt_get_config.c \
	ftd_mngt_registration_key_req.c \
	ftd_mngt_tdmf_cmd.c \
	ftd_mngt_agent_general_config.c \
	ftd_mngt_get_all_devices.c \
	ftd_mngt_get_perf_cfg.c \
	ftd_agn_get_rmd_stat.c \
    ftd_mngt_get_product_usage_data.c


Agent_msg.o:	tdmfAgent.h tdmfAgent_trace.h \
	Agent_msg.c

Agent_main.o:	tdmfAgent.h tdmfAgent_trace.h \
	Agent_main.c

Agent_config.o:	tdmfAgent.h tdmfAgent_trace.h \
	Agent_config.c

launchagent: launchagent.sh
	$(PKGIFY)
	@chmod +x $@

killagent: killagent.sh
	$(PKGIFY)
	@chmod +x $@

${PKGNAME}-startagent: startagent.sh.${SYSTYPE}
	$(PKGIFY)
	@chmod +x $@
