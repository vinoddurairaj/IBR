
CMD = pacewrite chkwrite
OBJ = $(foreach file, $(CMD), $(file).o)

PLATFORMDEFINES := $(shell uname -a | cut -d " " -f 1)
ifeq (HP-UX, $(PLATFORMDEFINES))
PLATFORMDEFINES := HPUX
endif
ifeq (SunOS, $(PLATFORMDEFINES))
PLATFORMDEFINES := SOLARIS
endif
ifeq (Linux, $(PLATFORMDEFINES))
PLATFORMDEFINES := linux
endif

LOCALCFLAGS = -D$(PLATFORMDEFINES)
CFLAGS += $(LOCALCFLAGS)

all: $(CMD)   

pacewrite : pacewrite.o
	cc -o pacewrite $(CFLAGS) pacewrite.o

chkwrite : chkwrite.o
	cc -o chkwrite $(CFLAGS) chkwrite.o

clean:
	rm -f $(OBJ)

clobber:
	rm -f $(OBJ) $(CMD)

chkwrite.o:	pacewrite.c
	$(CC) $(CFLAGS) -DVERIFY_ONLY -o $@ -c $<
