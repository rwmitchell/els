################################
# DEFAULT SETTINGS (user modifiable):
MAKE = make
CC = gcc
CPPFLAGS =
CFLAGS_DBUG = -g3 -DDEBUG_ALL
CFLAGS_GENERIC = -O
CFLAGS_GNU = -Wall
CFLAGS = -I$(SRC) $(CFLAGS_GENERIC) $(CFLAGS_GNU) -D$(OS_NAME)=$(OS_VERSION)
# CFLAGS =                   $(CFLAGS_GNU)   $(CFLAGS_DBUG)
LD = $(CC)
LDFLAGS = -lm
BINDIR = /usr/local/bin
MANEXT = 1
MANDIR = /usr/local/man/man$(MANEXT)

DIR = $(shell basename $(CURDIR))
BLD = Build
SRC = Source
BAS = $(BLD)/$(DIR)
DST = $(BAS)/bin
OBJ = $(BAS)/obj

DIRS =   \
	$(BAS) \
	$(OBJ) \
	$(DST) \

################################
# EXTRACT OS NAME:
# Delete bothersome characters such as [-/] and truncate certain OS names.
OS_NAME := $(shell uname -s | sed  -e 's;[-/];;g' -e 's/^\(CYGWIN\).*$$/\1/' -e 's/^\(AIX\).*$$/\1/' -e 's/^\(IRIX\).*$$/\1/' | tr [:lower:] [:upper:] )

################################
# EXTRACT OS VERSION:
# Given that output from "uname -r" == X.Y.Z, then OS_VERSION == XYYZZ
#	where Y and/or Z will be given leading zeros if less than 10;
#	if any fields are missing, then "00" will be used.
# For example:
#	if output from "uname -r" ==  5,       then OS_VERSION ==  50000
#	if output from "uname -r" ==  5.1,     then OS_VERSION ==  50100
#	if output from "uname -r" ==  5.12.3,  then OS_VERSION ==  51203
#	if output from "uname -r" ==  5.12.34, then OS_VERSION ==  51234
#	if output from "uname -r" == 15.12,    then OS_VERSION == 151200
#
OS_VERSION = $(shell uname -r | sed -e 's;$$; 0 0 0;' -e 's;[^0-9]; ;g' | awk '{ printf "%d%02d%02d", $$1, $$2, $$3 }')

################################

ELS_SRC = $(SRC)/els.c $(SRC)/getdate32.c $(SRC)/time32.c $(SRC)/auxil.c \
		$(SRC)/elsFilter.c $(SRC)/elsMisc.c $(SRC)/sysInfo.c $(SRC)/sysdep.c \
		$(SRC)/phLib.c $(SRC)/quotal.c $(SRC)/format.c $(SRC)/cksum.c $(SRC)/hg.c

CHD_SRC = $(SRC)/chdate.c $(SRC)/getdate32.c $(SRC)/time32.c $(SRC)/auxil.c $(SRC)/sysInfo.c

EDT_SRC = $(SRC)/edate.c $(SRC)/getdate32.c $(SRC)/time32.c $(SRC)/auxil.c $(SRC)/sysInfo.c

ELS_OBJ := $(ELS_SRC:%.c=%.o)
ELS_OBJ := $(ELS_OBJ:$(SRC)/%=$(OBJ)/%)

CHD_OBJ := $(CHD_SRC:%.c=%.o)
CHD_OBJ := $(CHD_OBJ:$(SRC)/%=$(OBJ)/%)

EDT_OBJ := $(EDT_SRC:%.c=%.o)
EDT_OBJ := $(EDT_OBJ:$(SRC)/%=$(OBJ)/%)

################################
# BUILD TARGETS:

default:
	@ echo "OS Version: " $(OS_VERSION)
	@ echo "OS Name   : " $(OS_NAME)
	@ echo $(MAKE) -d$(OS_NAME) $(OS_NAME)
	$(MAKE) -D$(OS_NAME) $(OS_NAME)

config: $(DIRS) $(SRC)/config.h


version:
	@ echo "OS Version: " $(OS_VERSION)
	@ echo "OS Name   : " $(OS_NAME)

# Identify phony targets:
.PHONY: default all install install_els5 clean realclean els5 \
	solaris SunOS SunOS5-cc SunOS5-cc32 \
	Linux CYGWIN \
	FreeBSD Darwin \
	HPUX HPUX-cc \
	OSF1 OSF1-cc \
	AIX  AIX-cc \
	IRIX sco SCO_SV ULTRIX \
	DYNIXptx ISC SYSV_OLD


all: $(DIRS) $(DST)/els $(DST)/chdate $(DST)/edate
	@ echo ""
	@ echo "All targets SUCCESSFULLY built"
	@ echo ""
	@ echo "SRC: " $(ELS_SRC)
	@ echo "OBJ: " $(ELS_OBJ)

$(DIRS):
	mkdir -p $@

install: default
	[ ! -d $(BINDIR) ] && mkdir -p $(BINDIR) || exit 0
	cp els chdate edate $(BINDIR)
	[ ! -d $(MANDIR) ] && mkdir -p $(MANDIR) || exit 0
	cp els.1 $(MANDIR)/els.$(MANEXT)
	cp chdate.1 $(MANDIR)/chdate.$(MANEXT)
	cp edate.1 $(MANDIR)/edate.$(MANEXT)
	@ echo ""
	@ echo "All targets SUCCESSFULLY installed"
	@ echo ""

# Optional els5 hardlink which uses SYS5 semantics instead of BSD semantics:
# (NB: In cygwin-land "[ -f els ]" is true if "els.exe" exists!)
install_els5:
	-@ rm -f $(BINDIR)/els5 $(BINDIR)/els5.exe
	-@ [ ! -f $(BINDIR)/els.exe ] && ln $(BINDIR)/els $(BINDIR)/els5
	-@ [ -f $(BINDIR)/els.exe ] && ln $(BINDIR)/els.exe $(BINDIR)/els5.exe

clean:
	rm -r $(OBJ)

realclean: clean
	rm -f els chdate edate els5

$(OBJ)/%.o	:	$(SRC)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(DST)/els: $(DST)/% : $(ELS_OBJ)
	@ echo
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DST)/chdate: $(DST)/% : $(CHD_OBJ)
	@ echo
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DST)/edate: $(DST)/% : $(EDT_OBJ)
	@ echo
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# els: els.o getdate32.o time32.o auxil.o \
# 		elsFilter.o elsMisc.o sysInfo.o sysdep.o phLib.o quotal.o \
# 		format.o cksum.o hg.o
# 	$(LD) els.o getdate32.o time32.o auxil.o \
# 		elsFilter.o elsMisc.o sysInfo.o sysdep.o phLib.o quotal.o \
# 		format.o cksum.o hg.o $(LDFLAGS) -o els
#
# chdate: chdate.o getdate32.o time32.o auxil.o sysInfo.o
# 	$(LD) $(LDFLAGS) chdate.o getdate32.o time32.o auxil.o sysInfo.o \
# 		-o chdate

# edate: edate.o getdate32.o time32.o auxil.o sysInfo.o
# 	$(LD) $(LDFLAGS) edate.o getdate32.o time32.o auxil.o sysInfo.o \
# 		-o edate

# Optional els5 hardlink which uses SYS5 semantics instead of BSD semantics:
# (NB: In cygwin-land "[ -f els ]" is true if "els.exe" exists!)
els5: els
	@ echo "rm -f els5; ln els els5"
	-@ rm -f els5 els5.exe
	-@ [ ! -f els.exe ] && ln els els5 || exit 0
	-@ [ -f els.exe ] && ln els.exe els5.exe || exit 0

################################
# OS TARGETS:

solaris \
SunOS:
	@ [ $(OS_VERSION) -lt 50700 ] && \
		$(MAKE) all \
		"CPPFLAGS = $(CPPFLAGS) -DSUNOS=$(OS_VERSION)" \
		|| exit 0
	@ [ $(OS_VERSION) -ge 50700 ] && \
		$(MAKE) all \
		"CPPFLAGS = $(CPPFLAGS) -DSUNOS=$(OS_VERSION)" \
		"CFLAGS = $(CFLAGS) -m64" \
		"LDFLAGS = $(LDFLAGS) -m64" \
		|| exit 0

SunOS5-cc:
	@ [ $(OS_VERSION) -lt 50700 ] && \
		$(MAKE) all \
		"CC = /opt/SUNWspro/bin/cc" \
		"CPPFLAGS = $(CPPFLAGS) -DSUNOS=$(OS_VERSION)" \
		"CFLAGS = $(CFLAGS_GENERIC)" \
		|| exit 0
	@ [ $(OS_VERSION) -ge 50700 ] && \
		$(MAKE) all \
		"CC = /opt/SUNWspro/bin/cc" \
		"CPPFLAGS = $(CPPFLAGS) -DSUNOS=$(OS_VERSION)" \
		"CFLAGS = $(CFLAGS_GENERIC) -xarch=v9" \
		"LDFLAGS = $(LDFLAGS) -xarch=v9" \
		|| exit 0

SunOS5-cc32:
	@ $(MAKE) all \
	"CC = /opt/SUNWspro/bin/cc" \
	"CPPFLAGS = $(CPPFLAGS) -DSUNOS=$(OS_VERSION)" \
	"CFLAGS = $(CFLAGS_GENERIC)"

Linux:
	@ [ ! -f /usr/include/sys/acl.h ] && \
		$(MAKE) all \
		"CPPFLAGS = $(CPPFLAGS) -DLINUX=$(OS_VERSION)" \
		|| exit 0
	@ [   -f /usr/include/sys/acl.h ] && \
		$(MAKE) all \
		"CPPFLAGS = $(CPPFLAGS) -DLINUX=$(OS_VERSION)" \
		"LDFLAGS = $(LDFLAGS) -lacl" \
		|| exit 0

CYGWIN:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DCYGWIN=$(OS_VERSION)"

FreeBSD:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DFREEBSD=$(OS_VERSION)"

Darwin:
	@ $(MAKE) all \
	"CC = cc" \
	"CPPFLAGS = $(CPPFLAGS) -DDARWIN=$(OS_VERSION)"

HPUX:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DHPUX=$(OS_VERSION)"

HPUX-cc:
	@ [ $(OS_VERSION) -lt 100000 ] && \
		$(MAKE) all \
		"CC = cc" \
		"CPPFLAGS = $(CPPFLAGS) -DHPUX=$(OS_VERSION)" \
		"CFLAGS = $(CFLAGS_GENERIC) -Aa" \
		|| exit 0
	@ [ $(OS_VERSION) -ge 100000 ] && \
		$(MAKE) all \
		"CC = cc" \
		"CPPFLAGS = $(CPPFLAGS) -DHPUX=$(OS_VERSION)" \
		"CFLAGS = $(CFLAGS_GENERIC) -Ae" \
		|| exit 0

OSF1:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DOSF1=$(OS_VERSION)" \
	"LDFLAGS = $(LDFLAGS) -lpacl"

OSF1-cc:
	@ $(MAKE) all \
	"CC = cc" \
	"CPPFLAGS = $(CPPFLAGS) -DOSF1=$(OS_VERSION)" \
	"CFLAGS = $(CFLAGS_GENERIC)" \
	"LDFLAGS = $(LDFLAGS) -lpacl"

AIX:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DAIX=$(OS_VERSION)" \

AIX-cc:
	@ $(MAKE) all \
	"CC = cc" \
	"CPPFLAGS = $(CPPFLAGS) -DAIX=$(OS_VERSION)" \
	"CFLAGS = $(CFLAGS_GENERIC)"

IRIX:
	@ $(MAKE) all \
	"CC = cc" \
	"CPPFLAGS = $(CPPFLAGS) -DIRIX=$(OS_VERSION)" \
	"CFLAGS = $(CFLAGS_GENERIC)"

sco \
SCO_SV:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DSCO=$(OS_VERSION)"

ULTRIX:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DULTRIX=$(OS_VERSION)"

DYNIXptx:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DDYNIX=$(OS_VERSION)" \
	"CFLAGS = $(CFLAGS_GENERIC)"

ISC \
SYSV_OLD:
	@ $(MAKE) all \
	"CPPFLAGS = $(CPPFLAGS) -DSYSV_OLD=$(OS_VERSION)" \
	"CFLAGS = $(CFLAGS_GENERIC)"

################################
# DEPENDENCIES:

$(SRC)/config.h: $(DST)/config
	$(DST)/config $(SRC)/config.h

$(DST)/config: $(OBJ)/config.o
	$(LD) $(LDFLAGS) $(OBJ)/config.o -o $(DST)/config

$(OBJ)/config.o: $(SRC)/config.c $(SRC)/version.h $(SRC)/sysdefs.h
	@ echo "OS_NAME: " $(OS_NAME) " : " $(OS_VERSION)
	@ echo $(CC) $(CFLAGS) $(CPPFLAGS) \
		-DOS_NAME=\"$(OS_NAME)\" -DOS_VERSION=$(OS_VERSION) \
		-c $(SRC)/config.c -o $(OBJ)/config.o
	$(CC) $(CFLAGS) $(CPPFLAGS) \
		-DOS_NAME=\"$(OS_NAME)\" -DOS_VERSION=$(OS_VERSION) \
		-c $(SRC)/config.c -o $(OBJ)/config.o

ifeq ( 0, 1)
els.o: els.c version.h sysdefs.h defs.h config.h getdate32.h time32.h \
	auxil.h els.h elsVars.h elsFilter.h elsMisc.h sysInfo.h sysdep.h \
	quotal.h format.h cksum.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c els.c
chdate.o: chdate.c version.h sysdefs.h defs.h config.h getdate32.h \
	auxil.h sysInfo.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c chdate.c
edate.o: edate.c version.h sysdefs.h defs.h config.h getdate32.h time32.h \
	auxil.h format.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c edate.c
getdate32.o: getdate32.c sysdefs.h defs.h config.h getdate32.h time32.h auxil.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c getdate32.c
time32.o: time32.c sysdefs.h defs.h config.h time32.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c time32.c
auxil.o: auxil.c sysdefs.h defs.h config.h auxil.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c auxil.c
elsFilter.o: elsFilter.c sysdefs.h defs.h config.h getdate32.h time32.h \
	auxil.h els.h elsVars.h elsFilter.h elsMisc.h sysdep.h phLib.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c elsFilter.c
elsMisc.o: elsMisc.c sysdefs.h defs.h config.h \
	auxil.h els.h elsVars.h elsMisc.h sysdep.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c elsMisc.c
sysInfo.o: sysInfo.c sysdefs.h defs.h config.h auxil.h sysInfo.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c sysInfo.c
sysdep.o: sysdep.c sysdefs.h defs.h config.h auxil.h \
	els.h elsVars.h elsMisc.h sysInfo.h sysdep.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c sysdep.c
phLib.o: phLib.c sysdefs.h defs.h config.h auxil.h phLib.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c phLib.c
quotal.o: quotal.c sysdefs.h defs.h config.h \
	auxil.h els.h elsVars.h elsMisc.h sysdep.h quotal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c quotal.c
format.o: format.c sysdefs.h defs.h config.h format.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c format.c
cksum.o: cksum.c sysdefs.h defs.h config.h auxil.h cksum.h elsVars.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c cksum.c
endif

################################
# EOF.
