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
CFLAGS = -O3 -Wall -std=gnu99
RESINC = 
RCFLAGS = 
LIBDIR =
LIB = ../libcloudthreads/bin/libcloudthreads.a -lcr -lssl -lcurl
LDFLAGS =

INC_PROFILE = $(INC)
CFLAGS_PROFILE = $(CFLAGS)
RESINC_PROFILE = $(RESINC)
RCFLAGS_PROFILE = $(RCFLAGS)
LIBDIR_PROFILE = $(LIBDIR)
LIB_PROFILE = $(LIB)
LDFLAGS_PROFILE = $(LDFLAGS)
OBJDIR_PROFILE = .obj/Profile/prodcons
DEP_PROFILE = 
OUT_PROFILE = ./bin/Profile/prodcons

INC_DEBUG = $(INC)
CFLAGS_DEBUG = $(CFLAGS) -g
RESINC_DEBUG = $(RESINC)
RCFLAGS_DEBUG = $(RCFLAGS)
LIBDIR_DEBUG = $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG = $(LDFLAGS)
OBJDIR_DEBUG = .obj/Debug/prodcons
DEP_DEBUG = 
OUT_DEBUG = ./bin/Debug/prodcons

INC_RELEASE = $(INC)
CFLAGS_RELEASE = $(CFLAGS)
RESINC_RELEASE = $(RESINC)
RCFLAGS_RELEASE = $(RCFLAGS)
LIBDIR_RELEASE = $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE = $(LDFLAGS)
OBJDIR_RELEASE = .obj/Release/prodcons
DEP_RELEASE = 
OUT_RELEASE = ./bin/Release/prodcons

OBJ_PROFILE = $(OBJDIR_PROFILE)/src/prodcons.o
OBJ_DEBUG = $(OBJDIR_DEBUG)/src/prodcons.o
OBJ_RELEASE = $(OBJDIR_RELEASE)/src/prodcons.o

all: profile debug release

clean: clean_profile clean_debug clean_release

profile: $(OUT_PROFILE)

$(OUT_PROFILE): $(OBJ_PROFILE) $(DEP_PROFILE)
	test -d ./bin/Profile || mkdir -p ./bin/Profile
	$(LD) $(LDFLAGS_PROFILE) $(LIBDIR_PROFILE) -o $(OUT_PROFILE) $(OBJ_PROFILE) $(LIB_PROFILE)

$(OBJDIR_PROFILE)/src/prodcons.o: src/prodcons.c
	test -d $(OBJDIR_PROFILE)/src || mkdir -p $(OBJDIR_PROFILE)/src
	$(CC) $(CFLAGS_PROFILE) $(INC_PROFILE) -c -o $(OBJDIR_PROFILE)/src/prodcons.o src/prodcons.c


clean_profile:
	rm -f $(OBJ_PROFILE) $(OUT_PROFILE)

debug: $(OUT_DEBUG)

$(OUT_DEBUG): $(OBJ_DEBUG) $(DEP_DEBUG)
	test -d ./bin/Debug || mkdir -p ./bin/Debug
	$(LD) $(LDFLAGS_DEBUG) $(LIBDIR_DEBUG) -o $(OUT_DEBUG) $(OBJ_DEBUG) $(LIB_DEBUG)

$(OBJDIR_DEBUG)/src/prodcons.o: src/prodcons.c
	test -d $(OBJDIR_DEBUG)/src || mkdir -p $(OBJDIR_DEBUG)/src
	$(CC) $(CFLAGS_DEBUG) $(INC_DEBUG) -c -o $(OBJDIR_DEBUG)/src/prodcons.o src/prodcons.c


clean_debug:
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)

release: $(OUT_RELEASE)

$(OUT_RELEASE): $(OBJ_RELEASE) $(DEP_RELEASE)
	test -d ./bin/Release || mkdir -p ./bin/Release
	$(LD) $(LDFLAGS_RELEASE) $(LIBDIR_RELEASE) -o $(OUT_RELEASE) $(OBJ_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/src/prodcons.o: src/prodcons.c
	test -d $(OBJDIR_RELEASE)/src || mkdir -p $(OBJDIR_RELEASE)/src
	$(CC) $(CFLAGS_RELEASE) $(INC_RELEASE) -c -o $(OBJDIR_RELEASE)/src/prodcons.o src/prodcons.c


clean_release:
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)

.PHONY: clean clean_profile clean_debug clean_release

