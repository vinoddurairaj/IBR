#
# Makefile for Agent
# Warning: Use /usr/local/bin/gmake
#

SHELL = /bin/sh

SYSTYPE:= $(shell uname -s)
SYSVERS:= $(shell ./unamevers.sh)

######### for Solaris ######### 
ifeq (SunOS,$(SYSTYPE))

CC = /opt/SUNWspro/bin/cc
LOCALDEF = -DQNM=\"dtc\" -DSYSVERS=${SYSVERS} -DSOLARIS
#LOCALDEF = -DQNM=\"dtc\" -DSYSVERS=${SYSVERS} -DSOLARIS -DAGN_TEST -DDEBUG
LDFLAGS = -lsocket -lnsl

endif

######### for AIX ######### 
ifeq (AIX,$(SYSTYPE))

CC = /usr/vac/bin/cc
LOCALDEF = -DQNM=\"dtc\" -D_AIX
#LOCALDEF = -DQNM=\"dtc\" -D_AIX -DAGN_TEST -DDEBUG
LDFLAGS = -llvm

endif

######### for HP-UX ######### 
ifeq (HP-UX,$(SYSTYPE))

CC = /bin/cc
LOCALDEF = -DQNM=\"dtc\" -DSYSVERS=${SYSVERS} -DHPUX -Ae +DAportable
#LOCALDEF = -DQNM=\"dtc\" -DSYSVERS=${SYSVERS} -DHPUX -DPATH_CONFIG=\"\" -DDEBUG
LDFLAGS =

endif


######### for Linux #########
ifeq (Linux,$(SYSTYPE))

CC = /usr/bin/cc
LOCALDEF = -DQNM=\"dtc\" -Dlinux
LDFLAGS =

endif




######### for common ######### 
PRODUCT = dtcAgent
ROOT = .
LOCALINC =
CFLAGS += $(LOCALDEF) $(LOCALINC)
I = /usr/include

AGENTOBJ =\
	iputil.o ftd_mngt.o Agent_uty.o Agent_sock.o Agent_proc.o \
	Agent_main.o license.o licplat.o

all: $(PRODUCT)

dtcAgent: $(AGENTOBJ)
	$(CC) $(CFLAGS) -o dtcAgent $(AGENTOBJ) $(LDFLAGS)


######### for Solaris ######### 
ifeq (SunOS,$(SYSTYPE))
iputil.o:	$(ROOT)/tdmfAgent.h \
		$I/stdio.h $I/syslog.h $I/errno.h $I/signal.h \
		$I/netdb.h $I/resolv.h $I/fcntl.h $I/unistd.h \
		$I/sys/types.h $I/sys/param.h $I/sys/file.h $I/stdlib.h \
		$I/sys/stat.h $I/sys/ioctl.h $I/sys/socket.h $I/netinet/in.h \
		$I/net/if.h $I/arpa/nameser.h $I/arpa/inet.h

ftd_mngt.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$(ROOT)/ftdio.h \
		$I/stdio.h $I/stdlib.h $I/sys/types.h $I/sys/utsname.h \
		$I/sys/syscall.h $I/sys/sysconfig.h $I/sys/stat.h \
		$I/sys/ipc.h $I/sys/sem.h $I/sys/socket.h $I/sys/errno.h \
		$I/sys/ioctl.h $I/sys/sockio.h $I/sys/ioccom.h \
		$I/netinet/in.h $I/arpa/inet.h $I/net/if_arp.h \
		$I/netdb.h $I/signal.h $I/unistd.h $I/string.h \
		$I/stdlib.h $I/fcntl.h $I/errno.h $I/ctype.h \
		$(ROOT)/ftd_mngt_gather_server_info.c \
		$(ROOT)/ftd_mngt_StatusMsgThread.c \
		$(ROOT)/ftd_mngt_performance_send_data.c \
		$(ROOT)/ftd_mngt_send_lg_state.c \
		$(ROOT)/ftd_mngt_sys_request_system_restart.c

Agent_uty.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/sem.h \
		$I/sys/socket.h $I/netinet/in.h $I/netdb.h $I/signal.h \
		$I/unistd.h $I/string.h $I/errno.h $I/time.h \
		$I/pwd.h $I/stdlib.h $I/macros.h $I/fcntl.h $I/sys/param.h \
		$I/sys/stat.h $I/sys/sysmacros.h

Agent_sock.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/socket.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/time.h $I/pwd.h $I/net/if_arp.h \
		$I/sys/ioctl.h $I/sys/sockio.h $I/sys/utsname.h

Agent_proc.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$(ROOT)/license.h $(ROOT)/ftdio.h \
		$I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/sem.h \
		$I/sys/utsname.h $I/sys/syscall.h $I/sys/sysconfig.h \
		$I/sys/socket.h $I/sys/stat.h $I/sys/wait.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/time.h $I/pwd.h $I/stdlib.h\
		$(ROOT)/ftd_mngt_set_config.c \
		$(ROOT)/ftd_mngt_get_config.c \
		$(ROOT)/ftd_mngt_registration_key_req.c \
		$(ROOT)/ftd_mngt_tdmf_cmd.c \
		$(ROOT)/ftd_mngt_agent_general_config.c \
		$(ROOT)/ftd_mngt_get_all_devices.c \
		$(ROOT)/ftd_mngt_get_perf_cfg.c \
		$(ROOT)/ftd_agn_get_rmd_stat.c

Agent_main.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/socket.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/time.h $I/pwd.h

license.o:	$(ROOT)/license.h \
		$I/stdio.h $I/stdlib.h $I/unistd.h $I/string.h \
		$I/sys/stat.h \
		$(ROOT)/licprod.c

licplat.o:	$I/stdio.h $I/sys/systeminfo.h 
endif

######### for AIX ######### 
ifeq (AIX,$(SYSTYPE))
iputil.o:	$(ROOT)/tdmfAgent.h \
		$I/stdio.h $I/syslog.h $I/errno.h $I/signal.h \
		$I/netdb.h $I/resolv.h $I/fcntl.h $I/unistd.h \
		$I/sys/types.h $I/sys/param.h $I/sys/file.h $I/stdlib.h \
		$I/sys/stat.h $I/sys/ioctl.h $I/sys/socket.h $I/netinet/in.h \
		$I/net/if.h $I/arpa/nameser.h $I/arpa/inet.h

ftd_mngt.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$(ROOT)/ftdio.h \
		$I/stdio.h $I/stdlib.h $I/sys/types.h $I/sys/utsname.h \
		$I/sys/sysconfig.h $I/sys/stat.h \
		$I/sys/ipc.h $I/sys/sem.h $I/sys/socket.h $I/sys/errno.h \
		$I/sys/ioctl.h $I/net/if.h $I/sys/param.h \
		$I/netinet/in.h $I/arpa/inet.h $I/net/if_arp.h \
		$I/netdb.h $I/signal.h $I/unistd.h $I/string.h \
		$I/stdlib.h $I/fcntl.h $I/errno.h $I/ctype.h \
		$(ROOT)/ftd_mngt_gather_server_info.c \
		$(ROOT)/ftd_mngt_StatusMsgThread.c \
		$(ROOT)/ftd_mngt_performance_send_data.c \
		$(ROOT)/ftd_mngt_send_lg_state.c \
		$(ROOT)/ftd_mngt_sys_request_system_restart.c

Agent_uty.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/sem.h \
		$I/sys/socket.h $I/netinet/in.h $I/netdb.h $I/signal.h \
		$I/unistd.h $I/string.h $I/errno.h $I/time.h \
		$I/pwd.h $I/stdlib.h $I/macros.h $I/fcntl.h $I/sys/param.h \
		$I/sys/stat.h $I/sys/sysmacros.h

Agent_sock.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/socket.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/time.h $I/pwd.h $I/net/if_arp.h \
		$I/sys/ioctl.h $I/sys/utsname.h $I/sys/un.h $I/net/if.h

Agent_proc.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$(ROOT)/license.h $(ROOT)/ftdio.h \
		$I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/sem.h \
		$I/sys/utsname.h $I/sys/sysconfig.h \
		$I/sys/socket.h $I/sys/stat.h $I/sys/wait.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/time.h $I/pwd.h $I/stdlib.h\
		$(ROOT)/ftd_mngt_set_config.c \
		$(ROOT)/ftd_mngt_get_config.c \
		$(ROOT)/ftd_mngt_registration_key_req.c \
		$(ROOT)/ftd_mngt_tdmf_cmd.c \
		$(ROOT)/ftd_mngt_agent_general_config.c \
		$(ROOT)/ftd_mngt_get_all_devices.c \
		$(ROOT)/ftd_mngt_get_perf_cfg.c \
		$(ROOT)/ftd_agn_get_rmd_stat.c

Agent_main.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/socket.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/sys/time.h $I/time.h $I/pwd.h

license.o:	$(ROOT)/license.h \
		$I/stdio.h $I/stdlib.h $I/unistd.h $I/string.h \
		$(ROOT)/licprod.c

licplat.o:	$I/memory.h $I/sys/utsname.h 
endif

######### for HP-UX ######### 
ifeq (HP-UX,$(SYSTYPE))
iputil.o:       $(ROOT)/tdmfAgent.h \
                $I/stdio.h $I/syslog.h $I/errno.h $I/signal.h \
                $I/netdb.h $I/resolv.h $I/fcntl.h $I/unistd.h \
                $I/sys/types.h $I/sys/param.h $I/stdlib.h \
                $I/sys/stat.h $I/sys/ioctl.h $I/sys/socket.h \
                $I/net/if.h $I/arpa/nameser.h $I/arpa/inet.h

ftd_mngt.o:     $(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
                $(ROOT)/ftdio.h \
                $I/stdio.h $I/stdlib.h $I/sys/types.h $I/sys/utsname.h \
                $I/sys/syscall.h $I/sys/stat.h \
                $I/sys/ipc.h $I/sys/sem.h $I/sys/socket.h $I/sys/errno.h \
                $I/sys/ioctl.h \
                $I/netinet/in.h $I/arpa/inet.h $I/net/if_arp.h \
                $I/netdb.h $I/signal.h $I/unistd.h $I/string.h \
                $I/stdlib.h $I/fcntl.h $I/errno.h $I/ctype.h \
                $(ROOT)/ftd_mngt_gather_server_info.c \
                $(ROOT)/ftd_mngt_StatusMsgThread.c \
                $(ROOT)/ftd_mngt_performance_send_data.c \
                $(ROOT)/ftd_mngt_send_lg_state.c

Agent_uty.o:    $(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
                $I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/sem.h \
                $I/sys/socket.h $I/netinet/in.h $I/netdb.h $I/signal.h \
                $I/unistd.h $I/string.h $I/errno.h $I/time.h \
                $I/pwd.h $I/stdlib.h $I/macros.h $I/fcntl.h $I/sys/param.h \
                $I/sys/stat.h $I/sys/sysmacros.h

Agent_sock.o:   $(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
                $I/stdio.h $I/sys/types.h $I/sys/socket.h \
                $I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
                $I/string.h $I/errno.h $I/time.h $I/pwd.h $I/net/if_arp.h \
                $I/sys/ioctl.h $I/sys/utsname.h

Agent_proc.o:   $(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
                $(ROOT)/license.h \
                $I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/sem.h \
                $I/sys/utsname.h $I/sys/syscall.h \
                $I/sys/socket.h $I/sys/stat.h $I/sys/wait.h \
                $I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
                $I/string.h $I/errno.h $I/time.h $I/pwd.h $I/stdlib.h\
                $(ROOT)/ftd_mngt_set_config.c \
                $(ROOT)/ftd_mngt_get_config.c \
                $(ROOT)/ftd_mngt_registration_key_req.c \
                $(ROOT)/ftd_mngt_tdmf_cmd.c \
                $(ROOT)/ftd_mngt_agent_general_config.c \
                $(ROOT)/ftd_mngt_get_all_devices.c \
                $(ROOT)/ftd_mngt_get_perf_cfg.c \
                $(ROOT)/ftd_agn_get_rmd_stat.c

Agent_main.o:   $(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
                $I/stdio.h $I/sys/types.h $I/sys/socket.h \
                $I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
                $I/string.h $I/errno.h $I/time.h $I/pwd.h

license.o:      $(ROOT)/license.h \
                $I/stdio.h $I/stdlib.h $I/unistd.h $I/string.h \
                $I/sys/stat.h \
                $(ROOT)/licprod.c

licplat.o:      $I/stdio.h
endif

######### for Linux ######### 
ifeq (Linux,$(SYSTYPE))
iputil.o:	$(ROOT)/tdmfAgent.h \
		$I/stdio.h $I/syslog.h $I/errno.h $I/signal.h \
		$I/netdb.h $I/resolv.h $I/fcntl.h $I/unistd.h \
		$I/sys/types.h $I/sys/param.h $I/sys/file.h $I/stdlib.h \
		$I/sys/stat.h $I/sys/ioctl.h $I/sys/socket.h $I/netinet/in.h \
		$I/net/if.h $I/arpa/nameser.h $I/arpa/inet.h

ftd_mngt.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$(ROOT)/ftdio.h \
		$I/stdio.h $I/stdlib.h $I/sys/types.h $I/sys/utsname.h \
		$I/sys/syscall.h $I/sys/stat.h \
		$I/sys/ipc.h $I/sys/sem.h $I/sys/socket.h $I/sys/errno.h \
		$I/sys/ioctl.h \
		$I/netinet/in.h $I/arpa/inet.h $I/net/if_arp.h \
		$I/netdb.h $I/signal.h $I/unistd.h $I/string.h \
		$I/stdlib.h $I/fcntl.h $I/errno.h $I/ctype.h \
		$(ROOT)/ftd_mngt_gather_server_info.c \
		$(ROOT)/ftd_mngt_StatusMsgThread.c \
		$(ROOT)/ftd_mngt_performance_send_data.c \
		$(ROOT)/ftd_mngt_send_lg_state.c \
		$(ROOT)/ftd_mngt_sys_request_system_restart.c

Agent_uty.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/sem.h \
		$I/sys/socket.h $I/netinet/in.h $I/netdb.h $I/signal.h \
		$I/unistd.h $I/string.h $I/errno.h $I/time.h \
		$I/pwd.h $I/stdlib.h $I/fcntl.h $I/sys/param.h \
		$I/sys/stat.h $I/sys/sysmacros.h

Agent_sock.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/socket.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/time.h $I/pwd.h $I/net/if_arp.h \
		$I/sys/ioctl.h $I/sys/utsname.h

Agent_proc.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$(ROOT)/license.h $(ROOT)/ftdio.h \
		$I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/sem.h \
		$I/sys/utsname.h $I/sys/syscall.h \
		$I/sys/socket.h $I/sys/stat.h $I/sys/wait.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/time.h $I/pwd.h $I/stdlib.h\
		$(ROOT)/ftd_mngt_set_config.c \
		$(ROOT)/ftd_mngt_get_config.c \
		$(ROOT)/ftd_mngt_registration_key_req.c \
		$(ROOT)/ftd_mngt_tdmf_cmd.c \
		$(ROOT)/ftd_mngt_agent_general_config.c \
		$(ROOT)/ftd_mngt_get_all_devices.c \
		$(ROOT)/ftd_mngt_get_perf_cfg.c \
		$(ROOT)/ftd_agn_get_rmd_stat.c

Agent_main.o:	$(ROOT)/tdmfAgent.h $(ROOT)/tdmfAgent_trace.h\
		$I/stdio.h $I/sys/types.h $I/sys/socket.h \
		$I/netinet/in.h $I/netdb.h $I/signal.h $I/unistd.h \
		$I/string.h $I/errno.h $I/time.h $I/pwd.h

license.o:	$(ROOT)/license.h \
		$I/stdio.h $I/stdlib.h $I/unistd.h $I/string.h \
		$I/sys/stat.h \
		$(ROOT)/licprod.c

licplat.o:	$I/stdio.h 
endif

clean:
	-rm -f $(AGENTOBJ) $(TESTCLTOBJ) $(TESTCOLLECTOBJ) $(LOGDUMPOBJ)

clobber: clean
	-rm -f $(PRODUCT)

