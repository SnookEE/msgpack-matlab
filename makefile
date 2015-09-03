REL_TOPDIR := .
OBJDIR = ./obj
MSGPACKROOT=../msgpack-c
MATLABROOT=/c/Program\ Files/MATLAB/R2014b
CC=gcc
CXX=g++
MEX_CFLAGS=-DMATLAB_MEX_FILE -I$(MATLABROOT)/extern/include 
EX_CFLAGS=  -Wall -m64 -O3 -I$(MSGPACKROOT)/include
EX_CPPFLAGS= $(EX_CFLAGS) -std=c++0x

MEX_LFLAGS=-shared -Wl,--export-all-symbols -L$(MATLABROOT)/bin/win64 -L$(MATLABROOT)/extern/lib/win64/microsoft
EX_LFLAGS=-L./ 

MEX_LIBS= -lmex -lmx -lmwlapack -lmwblas -leng
EX_LIBS=  -lgmp

MEX_TARG=msgpack.mexw64
CPP_EXEC_TARG=msgpack.exe

BI_OBJS	:= $(patsubst %.c, $(OBJDIR)/%.o, $(wildcard *.c)) $(patsubst %.cpp, $(OBJDIR)/%.o, $(wildcard *.cpp))

$(BI_OBJS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

-include $(wildcard $(OBJDIR)/*.d)

ifeq ($(VERBOSE),1)
  Q = 
else
  Q =
endif

MKDPND  := -MMD

all: exe mex
exe: CFLAGS   = $(EX_CFLAGS)
exe: CPPFLAGS = $(EX_CPPFLAGS)
exe: LFLAGS   = $(EX_LFLAGS) -static-libgcc -static-libstdc++
exe: LIBS     = $(EX_LIBS)
exe: $(CPP_EXEC_TARG)

mex: CFLAGS   = $(MEX_CFLAGS) $(EX_CFLAGS)
mex: CPPFLAGS = $(MEX_CFLAGS) $(EX_CPPFLAGS)
mex: LFLAGS   = $(MEX_LFLAGS) $(EX_LFLAGS) -static-libgcc -static-libstdc++
mex: LIBS     = $(MEX_LIBS)   $(EX_LIBS)
mex: $(MEX_TARG)

mex_clean: clean

$(MEX_TARG): $(BI_OBJS)
	@echo "	LD	`basename $@`"
	$(Q)$(CXX) $(LFLAGS) -o $@ $(BI_OBJS) $(LIBS)

$(EXEC_TARG): $(BI_OBJS)
	@echo "	LD	`basename $@`"
	$(Q)$(CC) $(LFLAGS) -o $@ $(BI_OBJS) $(LIBS)

$(CPP_EXEC_TARG): $(BI_OBJS)
	@echo "	LD	`basename $@`"
	$(Q)$(CXX) $(LFLAGS) -o $@ $(BI_OBJS) $(LIBS)

$(OBJDIR)/%.o : %.c
	@echo "	CC	`basename $@`"
	$(Q)$(CC) $(CFLAGS) $(EXTRA_USER_CFLAGS)  -c -o $@ $< $(MKDPND)

$(OBJDIR)/%.o : %.cpp
	@echo "	CPP	`basename $@`"
	$(Q)$(CXX) $(CPPFLAGS) $(EXTRA_USER_CFLAGS)  -c -o $@ $< $(MKDPND)

clean:
	@rm -rf $(BI_OBJS)
	@rm -rf $(MEX_TARG)
	@rm -rf $(EXEC_TARG)
	@rm -rf $(CPP_EXEC_TARG)
	@rm -rf $(OBJDIR)/*.d
	@if [ -d $(OBJDIR) ]; then rmdir --ignore-fail-on-non-empty $(OBJDIR); fi
  
.PHONY : all clean
