
#
# Makefile for Linux GNU compiler
#
# Usage:
#   make            - Build all tools. They will be copied into 'bin' directory.
#   make clean      - Clean.
#   make hlcsg      - Build hlcsg.
#   make hlbsp      - Build hlbsp.
#   make hlvis      - Build hlvis.
#   make hlrad      - Build hlrad.
#   make ripent     - Build ripent.
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
			hlcsg/ansitoutf8.cpp \
			hlcsg/autowad.cpp \
			hlcsg/brush.cpp \
			hlcsg/brushunion.cpp \
			hlcsg/hullfile.cpp \
			hlcsg/map.cpp \
			hlcsg/properties.cpp \
			hlcsg/qcsg.cpp \
			hlcsg/textures.cpp \
			hlcsg/wadcfg.cpp \
			hlcsg/wadinclude.cpp \
			hlcsg/wadpath.cpp \

HLCSG_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			hlcsg \

HLCSG_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			hlcsg/csg.h \
			hlcsg/wadpath.h \

HLCSG_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			HLCSG \
			DOUBLEVEC_T \
			
HLBSP_CPPFILES = \
			$(COMMON_CPPFILES) \
			hlbsp/brink.cpp \
			hlbsp/merge.cpp \
			hlbsp/outside.cpp \
			hlbsp/portals.cpp \
			hlbsp/qbsp.cpp \
			hlbsp/solidbsp.cpp \
			hlbsp/surfaces.cpp \
			hlbsp/tjunc.cpp \
			hlbsp/writebsp.cpp \

HLBSP_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			hlbsp \

HLBSP_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			hlbsp/bsp5.h \

HLBSP_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			HLBSP \
			DOUBLEVEC_T \

HLVIS_CPPFILES = \
			$(COMMON_CPPFILES) \
			hlvis/flow.cpp \
			hlvis/vis.cpp \
			hlvis/zones.cpp \

HLVIS_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			hlvis \

HLVIS_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			hlvis/vis.h \
			hlvis/zones.h \

HLVIS_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			HLVIS \

HLRAD_CPPFILES = \
			$(COMMON_CPPFILES) \
			hlrad/compress.cpp \
			hlrad/lerp.cpp \
			hlrad/lightmap.cpp \
			hlrad/loadtextures.cpp \
			hlrad/mathutil.cpp \
			hlrad/nomatrix.cpp \
			hlrad/qrad.cpp \
			hlrad/qradutil.cpp \
			hlrad/sparse.cpp \
			hlrad/trace.cpp \
			hlrad/transfers.cpp \
			hlrad/transparency.cpp \
			hlrad/vismatrix.cpp \
			hlrad/vismatrixutil.cpp \

HLRAD_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			hlrad \

HLRAD_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			common/anorms.h \
			hlrad/compress.h \
			hlrad/qrad.h \

HLRAD_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			HLRAD \

RIPENT_CPPFILES = \
			$(COMMON_CPPFILES) \
			ripent/ripent.cpp \

RIPENT_INCLUDEDIRS = \
			$(COMMON_INCLUDEDIRS) \
			ripent \

RIPENT_INCLUDEFILES = \
			$(COMMON_INCLUDEFILES) \
			ripent/ripent.h \

RIPENT_DEFINITIONS = \
			$(COMMON_DEFINITIONS) \
			RIPENT \

#
# Build commands
#

.PHONY : all
all : bin/hlcsg bin/hlbsp bin/hlvis bin/hlrad bin/ripent printusage
	@echo ======== OK ========

.PHONY : hlcsg
hlcsg : bin/hlcsg printusage
	@echo ======== OK ========

.PHONY : hlbsp
hlbsp : bin/hlbsp printusage
	@echo ======== OK ========

.PHONY : hlvis
hlvis : bin/hlvis printusage
	@echo ======== OK ========

.PHONY : hlrad
hlrad : bin/hlrad printusage
	@echo ======== OK ========

.PHONY : ripent
ripent : bin/ripent printusage
	@echo ======== OK ========

bin/hlcsg : $(HLCSG_CPPFILES:%.cpp=hlcsg/release/%.o) printusage
	@echo ======== hlcsg : linking ========
	mkdir -p hlcsg/release/bin
	g++ $(COMMON_FLAGS) -o hlcsg/release/bin/hlcsg $(addprefix -I,$(HLCSG_INCLUDEDIRS)) $(addprefix -D,$(HLCSG_DEFINITIONS)) $(HLCSG_CPPFILES:%.cpp=hlcsg/release/%.o)
	mkdir -p bin
	cp hlcsg/release/bin/hlcsg bin/hlcsg

$(HLCSG_CPPFILES:%.cpp=hlcsg/release/%.o) : hlcsg/release/%.o : %.cpp $(HLCSG_INCLUDEFILES) printusage
	@echo ======== hlcsg : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(HLCSG_INCLUDEDIRS)) $(addprefix -D,$(HLCSG_DEFINITIONS)) $<

bin/hlbsp : $(HLBSP_CPPFILES:%.cpp=hlbsp/release/%.o) printusage
	@echo ======== hlbsp : linking ========
	mkdir -p hlbsp/release/bin
	g++ $(COMMON_FLAGS) -o hlbsp/release/bin/hlbsp $(addprefix -I,$(HLBSP_INCLUDEDIRS)) $(addprefix -D,$(HLBSP_DEFINITIONS)) $(HLBSP_CPPFILES:%.cpp=hlbsp/release/%.o)
	mkdir -p bin
	cp hlbsp/release/bin/hlbsp bin/hlbsp

$(HLBSP_CPPFILES:%.cpp=hlbsp/release/%.o) : hlbsp/release/%.o : %.cpp $(HLBSP_INCLUDEFILES) printusage
	@echo ======== hlbsp : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(HLBSP_INCLUDEDIRS)) $(addprefix -D,$(HLBSP_DEFINITIONS)) $<

bin/hlvis : $(HLVIS_CPPFILES:%.cpp=hlvis/release/%.o) printusage
	@echo ======== hlvis : linking ========
	mkdir -p hlvis/release/bin
	g++ $(COMMON_FLAGS) -o hlvis/release/bin/hlvis $(addprefix -I,$(HLVIS_INCLUDEDIRS)) $(addprefix -D,$(HLVIS_DEFINITIONS)) $(HLVIS_CPPFILES:%.cpp=hlvis/release/%.o)
	mkdir -p bin
	cp hlvis/release/bin/hlvis bin/hlvis

$(HLVIS_CPPFILES:%.cpp=hlvis/release/%.o) : hlvis/release/%.o : %.cpp $(HLVIS_INCLUDEFILES) printusage
	@echo ======== hlvis : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(HLVIS_INCLUDEDIRS)) $(addprefix -D,$(HLVIS_DEFINITIONS)) $<

bin/hlrad : $(HLRAD_CPPFILES:%.cpp=hlrad/release/%.o) printusage
	@echo ======== hlrad : linking ========
	mkdir -p hlrad/release/bin
	g++ $(COMMON_FLAGS) -o hlrad/release/bin/hlrad $(addprefix -I,$(HLRAD_INCLUDEDIRS)) $(addprefix -D,$(HLRAD_DEFINITIONS)) $(HLRAD_CPPFILES:%.cpp=hlrad/release/%.o)
	mkdir -p bin
	cp hlrad/release/bin/hlrad bin/hlrad

$(HLRAD_CPPFILES:%.cpp=hlrad/release/%.o) : hlrad/release/%.o : %.cpp $(HLRAD_INCLUDEFILES) printusage
	@echo ======== hlrad : compiling $< ========
	mkdir -p $(dir $@)
	g++ -c $(COMMON_FLAGS) -o $@ $(addprefix -I,$(HLRAD_INCLUDEDIRS)) $(addprefix -D,$(HLRAD_DEFINITIONS)) $<

bin/ripent : $(RIPENT_CPPFILES:%.cpp=ripent/release/%.o) printusage
	@echo ======== ripent : linking ========
	mkdir -p ripent/release/bin
	g++ $(COMMON_FLAGS) -o ripent/release/bin/ripent $(addprefix -I,$(RIPENT_INCLUDEDIRS)) $(addprefix -D,$(RIPENT_DEFINITIONS)) $(RIPENT_CPPFILES:%.cpp=ripent/release/%.o)
	mkdir -p bin
	cp ripent/release/bin/ripent bin/ripent

$(RIPENT_CPPFILES:%.cpp=ripent/release/%.o) : ripent/release/%.o : %.cpp $(RIPENT_INCLUDEFILES) printusage
	@echo ======== ripent : compiling $< ========
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
	rm -rf hlcsg/release
	rm -rf hlbsp/release
	rm -rf hlvis/release
	rm -rf hlrad/release
	rm -rf ripent/release
	rm -rf bin
	@echo ======== OK ========


