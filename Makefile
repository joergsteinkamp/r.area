#MODULE_TOPDIR = ../../..
MODULE_TOPDIR = /usr/lib/grass72
PGM = r.area

LIBES=$(GISLIB) $(RASTERLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

#binary: main.c
#	if [ ! -d "$(OBJDIR)" ];then \
#	  mkdir $(OBJDIR);           \
#	fi
#	$(CC) -I$(MODULE_TOPDIR)/include -Wall -g -O2 -DPACKAGE=\""grassmods"\" -o $(OBJDIR)/main.o -c main.c
#	$(CC) -L$(MODULE_TOPDIR)/lib -Wl,--export-dynamic -Wl,-rpath-link,$(MODULE_TOPDIR)/lib -o $(PGM) $(OBJDIR)/main.o  $(GISLIB) $(MATHLIB)
#
#js: binary
#	$(CC) -L$(MODULE_TOPDIR)/lib -Wl,--export-dynamic -Wl,-rpath-link,$(MODULE_TOPDIR)/lib -o $(PGM) $(OBJDIR)/main.o  $(GISLIB) $(MATHLIB)
