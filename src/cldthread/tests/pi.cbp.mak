#------------------------------------------------------------------------------#
# This makefile was generated by 'cbp2make' tool rev.80                        #
#------------------------------------------------------------------------------#

WRKDIR = `pwd`


CC = gcc
CPP = g++
F77 = f77
F9X = gfortran
LD = g++
AR = ar
RANLIB = ranlib
WINDRES = windres

INC = -I../libcloudthreads/src
CFLAGS = -Wall -std=gnu99
RESINC = 
RCFLAGS = 
LIBDIR =
LIB = ../libcloudthreads/bin/libcloudthreads.a -lcr -lssl -lcurl
LDFLAGS =

INC_DEBUG = $(INC)
CFLAGS_DEBUG = $(CFLAGS) -g
RESINC_DEBUG = $(RESINC)
RCFLAGS_DEBUG = $(RCFLAGS)
LIBDIR_DEBUG = $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG = $(LDFLAGS)
OBJDIR_DEBUG = .objs
DEP_DEBUG = 
OUT_DEBUG = ./bin/pi

INC_RELEASE = $(INC)
CFLAGS_RELEASE = $(CFLAGS)
RESINC_RELEASE = $(RESINC)
RCFLAGS_RELEASE = $(RCFLAGS)
LIBDIR_RELEASE = $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE = $(LDFLAGS)
OBJDIR_RELEASE = .objs
DEP_RELEASE = 
OUT_RELEASE = ./bin/pi

OBJ_DEBUG = $(OBJDIR_DEBUG)/src/pi.o
OBJ_RELEASE = $(OBJDIR_RELEASE)/src/pi.o

all: debug release

clean: clean_debug clean_release

debug: $(OUT_DEBUG)

$(OUT_DEBUG): $(OBJ_DEBUG) $(DEP_DEBUG)
	test -d ./bin || mkdir -p ./bin
	$(LD) $(LDFLAGS_DEBUG) $(LIBDIR_DEBUG) -o $(OUT_DEBUG) $(OBJ_DEBUG) $(LIB_DEBUG)

$(OBJDIR_DEBUG)/src/pi.o: src/pi.c
	test -d $(OBJDIR_DEBUG)/src || mkdir -p $(OBJDIR_DEBUG)/src
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c -o $(OBJDIR_DEBUG)/src/pi.o src/pi.c


clean_debug:
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)

release: $(OUT_RELEASE)

$(OUT_RELEASE): $(OBJ_RELEASE) $(DEP_RELEASE)
	test -d ./bin || mkdir -p ./bin
	$(LD) $(LDFLAGS_RELEASE) $(LIBDIR_RELEASE) -o $(OUT_RELEASE) $(OBJ_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/src/pi.o: src/pi.c
	test -d $(OBJDIR_RELEASE)/src || mkdir -p $(OBJDIR_RELEASE)/src
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c -o $(OBJDIR_RELEASE)/src/pi.o src/pi.c


clean_release:
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)

.PHONY: clean clean_debug clean_release

