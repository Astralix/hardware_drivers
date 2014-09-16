#------------------------------
#
# XpndrCli
#
#------------------------------

MAKE	= make -e
MKDIR	= mkdir -p
RM	= rm -f

# should sub-make be called with '-s' (silent)
ifeq "$(VERBOSE)" "no"
 MAKE += -s
endif

# If no configuration is specified, "linux-arm-gcc" will be used
ifndef "CFG"
CFG=linux-arm-gcc-release
endif

#
# Configuration: linux-arm-gcc
#
ifeq "$(CFG)" "linux-arm-gcc"
CFG_DEF=
CFG_INC=
CFG_OBJ=
endif

#
# Configuration: linux-arm-gcc-release
#
ifeq "$(CFG)" "linux-arm-gcc-release"
CFG_DEF=
CFG_INC=
CFG_OBJ=
endif

#
# Configuration: linux_x86_gcc
#
ifeq "$(CFG)" "linux_x86_gcc"
CFG_DEF=
CFG_INC=
CFG_OBJ=
endif

OUTDIR	= ../../bin/$(CFG)

OUTFILE=$(OUTDIR)/XpndrCli

COMMON_INC=\
	-I../../include 				\
	-I../../src					\
	-I$(STAGING_DIR)/include 			\
	-I$(STAGING_DIR)/include/OsWrapperLite/Include	\
	-I$(STAGING_DIR)/include/infra/include


COMMON_OBJ=\
	   	$(OUTDIR)/Main.o \
		$(OUTDIR)/MenuCommands.o \
		$(OUTDIR)/RxSettings.o \
		$(OUTDIR)/TxPacketSettings.o \
		$(OUTDIR)/WifiCLICommand.o \
		$(OUTDIR)/XpndrCli.o

CFG_LIB= -lxpndr-wm-testmgr -lxpndr-wm -lwpa_ctrl -lxpndr-infra -lxpndr-climenu \
	-lxpndr-syssettings -lsqlitewrapped -lsqlite3 -lpthread -lrt -ldl

OBJ=$(CFG_OBJ) $(COMMON_OBJ)
INC=$(CFG_INC) $(COMMON_INC)
ALL_OBJ=$(OBJ) $(CFG_LIB)



LOCAL_DEF="-DAPP_LINUX_TEMPLATE_APP_ARM"




DEF=$(GLOBAL_DEF) $(LOCAL_DEF) $(CFG_DEF) $(COMMON_DEF)


COMPILE=$(CC) -c $(INC) $(DEF) $(CFLAGS) -o "$(OUTDIR)/$(*F).o" "$<"
COMPILE_CXX=$(CXX) -c $(INC) $(DEF) $(CXXFLAGS) -o "$(OUTDIR)/$(*F).o" "$<"

LINK= $(CXX) -o "$(OUTFILE)" $(ALL_OBJ)			\
		-L$(STAGING_DIR)/lib -uBP_putconsole -uConsoleTask


# Build rules
all:  prebuildcmds $(OUTDIR) $(OBJ) 
	$(LINK)

prebuildcmds:
	$(RM) $(OUTFILE)


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
$(OUTDIR)/%.o : ../../src/%.cpp ../../src/*.h
	$(COMPILE_CXX)


