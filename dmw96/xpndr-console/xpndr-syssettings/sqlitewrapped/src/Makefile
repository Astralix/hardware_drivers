# Make command to use for dependencies
MAKE=make
RM=rm
MKDIR=mkdir
INSTALL=install -v

# If no configuration is specified, "linux-arm-gcc-release" will be used
ifndef "CFG"
CFG=linux-arm-gcc
endif


#
# Configuration: Linux_Arm_Versa_Debug
#
ifeq "$(CFG)" "linux-arm-gcc"
CFG_DEF=
CFG_INC=-I$(STAGING_DIR)/include
CFG_OBJ=
endif

#
# Configuration: Linux_Arm_Versa_Release
#
ifeq "$(CFG)" "linux-arm-gcc-release"
CFG_DEF=
CFG_INC=
CFG_OBJ=
endif

#
# Configuration: Linux_x86_Debug
#
ifeq "$(CFG)" "linux-x86-gcc"
CFG_DEF=
CFG_INC=
CFG_OBJ=
endif

LIBNAME=libsqlitewrapped
OUTDIR=.
OUTFILE=$(OUTDIR)/$(LIBNAME).a

COMMON_INC=

COMMON_OBJ=\
	$(OUTDIR)/SysLog.o		\
	$(OUTDIR)/StderrLog.o		\
	$(OUTDIR)/Database.o 		\
	$(OUTDIR)/Query.o		\
	$(OUTDIR)/SQLiteBinary.o


COMMON_DEF=

OBJ=$(COMMON_OBJ) $(CFG_OBJ)

DEF=$(GLOBAL_DEF) $(CFG_DEF) $(COMMON_DEF)
INC=$(CFG_INC) $(COMMON_INC)

COMPILE= $(CC) -c -x c++ $(INC) $(DEF) \
	-O0 -g -ggdb -Wall -W -Wno-non-virtual-dtor -o "$(OUTDIR)/$(*F).o" "$<"

LINK= $(AR) -rs "$(OUTFILE)" $(OBJ)

# Pattern rules

$(OUTDIR)/%.o : ./%.cpp
	$(COMPILE)

# Build rules
all: $(OUTFILE)


$(OUTFILE): $(OUTDIR) $(OBJ)
	$(LINK)
	cat IError.h StderrLog.h SysLog.h Database.h Query.h SQLiteBinary.h > $(LIBNAME).h

$(OUTDIR):
	$(MKDIR) -p "$(OUTDIR)"

install:
	$(INSTALL) $(OUTFILE) $(prefix)/lib/$(LIBNAME).a
	$(INSTALL) libsqlitewrapped.h $(prefix)/include/libsqlitewrapped.h


# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	$(RM) -f $(OUTFILE)
	$(RM) -f $(OBJ)

# Clean this project and all dependencies
cleanall: clean

uninstall: clean
