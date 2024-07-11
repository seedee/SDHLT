
#
# Makefile for Linux GNU compiler
#
# Usage:
#   make            - Build all tools. They will be copied into 'bin' directory.
#   make clean      - Clean.
#   make sdHLCSG      - Build sdHLCSG.
#   make sdHLBSP      - Build sdHLBSP.
#   make sdHLVIS      - Build sdHLVIS.
#   make sdHLRAD      - Build sdHLRAD.
#   make sdRIPENT     - Build sdRIPENT.
#
# Before running the tools, please make sure the default maximum stack size on your computer
#   is more than 4MB.
#
# When compiling with g++:
#   Flag '-fno-strict-aliasing' is a must.
#   All macro definitions should not be changed, because all code were written and checked under this assumption.
#   The following warnings should be ignored:
#     warning: no newline at end of file
#     warning: '???' may be used uninitialized in this function
#     warning: suggest parentheses around assignment used as truth value
#     warning: passing ‘float’ for argument 1 to ‘void seconds_to_hhmm(unsigned int, unsigned int&, unsigned int&, unsigned int&, unsigned int&)’
#     warning: ignoring #pragma warning
#     warning: comparison between signed and unsigned integer expressions
#     warning: enumeration value ‘???’ not handled in switch
#     warning: unused variable ‘???’
#     warning: converting to ‘int’ from ‘vec_t’
#








#
# Common .cpp and .h files for all projects
#

COMMON_CPPFILES = \
			common/blockmem.cpp \
			common/bspfile.cpp \
			common/cmdlib.cpp \
			common/cmdlinecfg.cpp \
			common/filelib.cpp \
			common/log.cpp \
			common/mathlib.cpp \
			common/messages.cpp \
			common/scriplib.cpp \
			common/threads.cpp \
			common/winding.cpp \

COMMON_INCLUDEDIRS = \
			template \
			common \

COMMON_INCLUDEFILES = \
			template/basictypes.h \
			common/blockmem.h \
			common/boundingbox.h \
			common/bspfile.h \
			common/cmdlib.h \
			common/cmdlinecfg.h \
			common/filelib.h \
			common/hlassert.h \
			common/log.h \
			common/mathlib.h \
			common/mathtypes.h \
			common/messages.h \
			common/scriplib.h \
			common/threads.h \
			common/win32fix.h \
			common/winding.h \

COMMON_DEFINITIONS = \
			VERSION_LINUX \
			SYSTEM_POSIX \
			NDEBUG \
			STDC_HEADERS \
			HAVE_FCNTL_H \
			HAVE_PTHREAD_H \
			HAVE_SYS_RESOURCE_H \
			HAVE_SYS_STAT_H \
			HAVE_SYS_TIME_H \
			HAVE_UNISTD_H \

COMMON_FLAGS = -Wall -O2 -fno-strict-aliasing -pthread -pipe

#
# Specific .cpp and .h files for hlcsg, hlbsp, hlvis, hlrad and ripent
#

HLCSG_CPPFILES = \
			$(COMMON_CPPFILES) \
			sdHLCSG/ansitoutf8.cpp \
			sdHLCSG/brush.cpp \
			sdHLCSG/brushunion.cpp \
			sdHLCSG/hullfile.cpp \
			sdHLCSG/map.cpp \
			sdHLCSG/properties.cpp \
			sdHLCSG/qcsg.cpp \
			sdHLCSG/textures.cpp \
			sdHLCSG/wadcfg.cpp \
			sdHLCSG/wadpath.cpp \

HLCSG_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			sdHLCSG \

HLCSG_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			sdHLCSG/csg.h \
			sdHLCSG/wadpath.h \

HLCSG_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			SDHLCSG \
			DOUBLEVEC_T \
			
HLBSP_CPPFILES = \
			$(COMMON_CPPFILES) \
			sdHLBSP/brink.cpp \
			sdHLBSP/merge.cpp \
			sdHLBSP/outside.cpp \
			sdHLBSP/portals.cpp \
			sdHLBSP/qbsp.cpp \
			sdHLBSP/solidbsp.cpp \
			sdHLBSP/surfaces.cpp \
			sdHLBSP/tjunc.cpp \
			sdHLBSP/writebsp.cpp \

HLBSP_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			sdHLBSP \

HLBSP_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			sdHLBSP/bsp5.h \

HLBSP_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			SDHLBSP \
			DOUBLEVEC_T \

HLVIS_CPPFILES = \
			$(COMMON_CPPFILES) \
			sdHLVIS/flow.cpp \
			sdHLVIS/vis.cpp \
			sdHLVIS/zones.cpp \

HLVIS_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			sdHLVIS \

HLVIS_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			sdHLVIS/vis.h \
			sdHLVIS/zones.h \

HLVIS_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			SDHLVIS \

HLRAD_CPPFILES = \
			$(COMMON_CPPFILES) \
			sdHLRAD/compress.cpp \
			sdHLRAD/lerp.cpp \
			sdHLRAD/lightmap.cpp \
			sdHLRAD/loadtextures.cpp \
			sdHLRAD/mathutil.cpp \
			sdHLRAD/meshdesc.cpp \
			sdHLRAD/meshtrace.cpp \
			sdHLRAD/nomatrix.cpp \
			sdHLRAD/progmesh.cpp \
			sdHLRAD/qrad.cpp \
			sdHLRAD/qradutil.cpp \
			sdHLRAD/sparse.cpp \
			sdHLRAD/stringlib.cpp \
			sdHLRAD/studio.cpp \
			sdHLRAD/trace.cpp \
			sdHLRAD/transfers.cpp \
			sdHLRAD/transparency.cpp \
			sdHLRAD/vismatrix.cpp \
			sdHLRAD/vismatrixutil.cpp \

HLRAD_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			sdHLRAD \

HLRAD_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			common/anorms.h \
			sdHLRAD/compress.h \
			sdHLRAD/list.h \
			sdHLRAD/meshdesc.h \
			sdHLRAD/meshtrace.h \
			sdHLRAD/qrad.h \

HLRAD_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			SDHLRAD \

RIPENT_CPPFILES = \
			$(COMMON_CPPFILES) \
			sdRIPENT/ripent.cpp \

RIPENT_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			sdRIPENT \

RIPENT_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			sdRIPENT/ripent.h \

RIPENT_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			SDRIPENT \

#
# Build commands
#

.PHONY : all
all : bin/sdHLCSG bin/sdHLBSP bin/sdHLVIS bin/sdHLRAD bin/sdRIPENT printusage
	@echo ======== OK ========

.PHONY : sdHLCSG
sdHLCSG : bin/sdHLCSG printusage
	@echo ======== OK ========

.PHONY : sdHLBSP
sdHLBSP : bin/sdHLBSP printusage
	@echo ======== OK ========

.PHONY : sdHLVIS
sdHLVIS : bin/sdHLVIS printusage
	@echo ======== OK ========

.PHONY : sdHLRAD
sdHLRAD : bin/sdHLRAD printusage
	@echo ======== OK ========

.PHONY : sdRIPENT
sdRIPENT : bin/sdRIPENT printusage
	@echo ======== OK ========

bin/sdHLCSG : $(HLCSG_CPPFILES:%.cpp=sdHLCSG/release/%.o) printusage
	@echo ======== sdHLCSG : linking ========
	mkdir -p sdHLCSG/release/bin
	g++ $(COMMON_FLAGS) -o sdHLCSG/release/bin/sdHLCSG $(addprefix -I,$(HLCSG_INCLUDEDIRS)) $(addprefix -D,$(HLCSG_DEFINITIONS)) $(HLCSG_CPPFILES:%.cpp=sdHLCSG/release/%.o)
	mkdir -p bin
	cp sdHLCSG/release/bin/sdHLCSG bin/sdHLCSG

$(HLCSG_CPPFILES:%.cpp=sdHLCSG/release/%.o) : sdHLCSG/release/%.o : %.cpp $(HLCSG_INCLUDEFILES) printusage
	@echo ======== sdHLCSG : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(HLCSG_INCLUDEDIRS)) $(addprefix -D,$(HLCSG_DEFINITIONS)) $<

bin/sdHLBSP : $(HLBSP_CPPFILES:%.cpp=sdHLBSP/release/%.o) printusage
	@echo ======== sdHLBSP : linking ========
	mkdir -p sdHLBSP/release/bin
	g++ $(COMMON_FLAGS) -o sdHLBSP/release/bin/sdHLBSP $(addprefix -I,$(HLBSP_INCLUDEDIRS)) $(addprefix -D,$(HLBSP_DEFINITIONS)) $(HLBSP_CPPFILES:%.cpp=sdHLBSP/release/%.o)
	mkdir -p bin
	cp sdHLBSP/release/bin/sdHLBSP bin/sdHLBSP

$(HLBSP_CPPFILES:%.cpp=sdHLBSP/release/%.o) : sdHLBSP/release/%.o : %.cpp $(HLBSP_INCLUDEFILES) printusage
	@echo ======== sdHLBSP : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(HLBSP_INCLUDEDIRS)) $(addprefix -D,$(HLBSP_DEFINITIONS)) $<

bin/sdHLVIS : $(HLVIS_CPPFILES:%.cpp=sdHLVIS/release/%.o) printusage
	@echo ======== sdHLVIS : linking ========
	mkdir -p sdHLVIS/release/bin
	g++ $(COMMON_FLAGS) -o sdHLVIS/release/bin/sdHLVIS $(addprefix -I,$(HLVIS_INCLUDEDIRS)) $(addprefix -D,$(HLVIS_DEFINITIONS)) $(HLVIS_CPPFILES:%.cpp=sdHLVIS/release/%.o)
	mkdir -p bin
	cp sdHLVIS/release/bin/sdHLVIS bin/sdHLVIS

$(HLVIS_CPPFILES:%.cpp=sdHLVIS/release/%.o) : sdHLVIS/release/%.o : %.cpp $(HLVIS_INCLUDEFILES) printusage
	@echo ======== sdHLVIS : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(HLVIS_INCLUDEDIRS)) $(addprefix -D,$(HLVIS_DEFINITIONS)) $<

bin/sdHLRAD : $(HLRAD_CPPFILES:%.cpp=sdHLRAD/release/%.o) printusage
	@echo ======== sdHLRAD : linking ========
	mkdir -p sdHLRAD/release/bin
	g++ $(COMMON_FLAGS) -o sdHLRAD/release/bin/sdHLRAD $(addprefix -I,$(HLRAD_INCLUDEDIRS)) $(addprefix -D,$(HLRAD_DEFINITIONS)) $(HLRAD_CPPFILES:%.cpp=sdHLRAD/release/%.o)
	mkdir -p bin
	cp sdHLRAD/release/bin/sdHLRAD bin/sdHLRAD

$(HLRAD_CPPFILES:%.cpp=sdHLRAD/release/%.o) : sdHLRAD/release/%.o : %.cpp $(HLRAD_INCLUDEFILES) printusage
	@echo ======== sdHLRAD : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(HLRAD_INCLUDEDIRS)) $(addprefix -D,$(HLRAD_DEFINITIONS)) $<

bin/sdRIPENT : $(RIPENT_CPPFILES:%.cpp=sdRIPENT/release/%.o) printusage
	@echo ======== sdRIPENT : linking ========
	mkdir -p sdRIPENT/release/bin
	g++ $(COMMON_FLAGS) -o sdRIPENT/release/bin/sdRIPENT $(addprefix -I,$(RIPENT_INCLUDEDIRS)) $(addprefix -D,$(RIPENT_DEFINITIONS)) $(RIPENT_CPPFILES:%.cpp=sdRIPENT/release/%.o)
	mkdir -p bin
	cp sdRIPENT/release/bin/sdRIPENT bin/sdRIPENT

$(RIPENT_CPPFILES:%.cpp=sdRIPENT/release/%.o) : sdRIPENT/release/%.o : %.cpp $(RIPENT_INCLUDEFILES) printusage
	@echo ======== sdRIPENT : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(RIPENT_INCLUDEDIRS)) $(addprefix -D,$(RIPENT_DEFINITIONS)) $<

.PHONY : printusage
printusage :
	head -n 35 Makefile

#
# Clean
#

.PHONY : clean
clean : printusage
	rm -rf sdHLCSG/release
	rm -rf sdHLBSP/release
	rm -rf sdHLVIS/release
	rm -rf sdHLRAD/release
	rm -rf sdRIPENT/release
	rm -rf bin
	@echo ======== OK ========


