# Make command to use for dependencies
MAKE	= make -e
MKDIR	= mkdir -p
RM	= rm -f

# If no configuration is specified, "linux-arm-gcc-release" will be used
ifndef "CFG"
CFG=linux-arm-gcc-release
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
CFG_INC=-I$(STAGING_DIR)/include
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

OUTDIR=../../bin/$(CFG)
OUTFILE=$(OUTDIR)/libCliMenu.a

COMMON_INC=\
	-I../../src 					\
	-I../../include 				\
	-I$(STAGING_DIR)/include/OsWrapperLite/Include
	

CFG_LIB=
CFG_OBJ=
COMMON_OBJ=\
		$(OUTDIR)/MenuEntry.o			\
		$(OUTDIR)/ActionCommand.o		\
		$(OUTDIR)/ActionCallbackCommand.o	\
		$(OUTDIR)/SubMenu.o			\
		$(OUTDIR)/Command.o			\
		$(OUTDIR)/CliUtils.o			\
		$(OUTDIR)/CliMenu.o				

COMMON_DEF=

OBJ=$(COMMON_OBJ) $(CFG_OBJ)

ALL_OBJ=$(OBJ)

LOCAL_DEF=


DEF=$(GLOBAL_DEF) $(LOCAL_DEF) $(CFG_DEF) $(COMMON_DEF)
INC=$(GLOBAL_INC) $(CFG_INC) $(COMMON_INC)

COMPILE_CXX=$(CXX) -c $(INC) $(DEF) $(CXXFLAGS) -o "$(OUTDIR)/$(*F).o" "$<"

#LINK= $(AR) -rs "$(OUTFILE)" $(OBJ) 

#LINK= $(LD) "$(OUTFILE)" $(ALL_OBJ)  $(LD_SYS_LIB_PATH)



# Build rules
all: prebuildcmds $(OUTDIR) $(OBJ)
#	$(LINK)

prebuildcmds:
#	$(RM) -f $(OUTFILE)

$(OUTDIR):
	$(MKDIR) "$(OUTDIR)"

# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	$(RM) $(OUTFILE)
	$(RM) $(OBJ)

# Clean this project and all dependencies
cleanall: clean


# Pattern rules
$(OUTDIR)/%.o : ../../src/%.cpp ../../include/*.h ../../src/*.h
	$(COMPILE_CXX)

