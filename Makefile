# GRANIITTI Makefile
#
# (c) 2017-2021 Mikael Mieskolainen.
# Licensed under the MIT License <http://opensource.org/licenses/MIT>.
# -----------------------------------------------------------------------
#
# COMPILING:
#        make -j4
#
# CLEANING:
#        make clean (objects)
#        make superclean (objects + binaries)
#
# To compile with clang:                    make CXX=clang
#     -|-         unit tests:               make TEST=TRUE
# 
# To compile with different c++ standards:  make CXX_STANDARD=c++2a (c++17, c++20)
# 
# To compile with ROOT using -std=c++17:    make CXX_ROOT=c++17
# 
# To compile without ROOT                   make ROOT=
#
# To compile with profiling:                make VALGRIND=TRUE
# 
# -----------------------------------------------------------------------
#
# EXTERNAL LIBRARIES SETUP:
#
#   [HEPMC3] and [LHAPDF]: See ./install folder
#   [ROOT]                 Example: export ROOTSYS=/home/user/local/ROOT6
#
# GENERAL:
#
#   If you just installed e.g. HEPMC3 libraries
#   $> sudo ldconfig
# 
# -----------------------------------------------------------------------
# TIPS:
# 
# If you see symbol errors related to HepMC such that:
# "undefined symbol: _ZN5HepMC10FilterBase19init_has_end_vertexEv"
# check that you do not have a collision of HEPMC3 with an existing
# HepMC2 shared library installation!
#
# Check below:
#
# For tracking the libraries of the program:
#  $> ldd ./executable
#
# Check if the shared object (.so) file contains the missing symbol:
#  $> nm -D libHepMC.so
# 
# .......................................................................
#
# Creating shared libraries, use -fPIC flag, then:
# g++ -shared MH1.o -o libMH1.so
# 
# .......................................................................
#
# Enable MACRO switches via 'g++ -DNAME', where NAME e.g. DEBUG
# 
# .......................................................................
#
# Find out the compiler version of libraries (works with gcc, at least)
# 
#  $> readelf -p .comment ./obj/MGraniitti.o
# 
#
# -----------------------------------------------------------------------
# USE [TABS] for intendation while modifying this file!
# -----------------------------------------------------------------------


# =======================================================================
# Detect compiler version if using by default g++

ifeq ($(CXX),g++)

# KERNEL_GCC_VERSION
# := $(shell cat /proc/version | sed -n 's/.*gcc version \([[:digit:]]\.[[:digit:]]\.[[:digit:]]\).*/\1/p')
# $(info Info KERNEL_GCC_VERSION = $(KERNEL_GCC_VERSION))

CXX_VERSION = $(shell g++ -dumpversion)
$(info Found CXX_VERSION = $(CXX_VERSION))
CXX_REQUIRED = 7.0

# Use bc command for double comparison, a common utility on Unix platforms
FOO=$(shell echo "$(CXX_VERSION) < $(CXX_REQUIRED)" | bc)

ifeq ($(FOO),1)
$(error Your CXX_VERSION is too old, need at least CXX_VERSION = $(CXX_REQUIRED), update your GCC chain)
endif

endif


# =======================================================================
# PATH setup

# HEPMC3 installation
#HEPMC3SYS      = $(shell HepMC-config --prefix)

# LHAPDF installation
#LHAPDFSYS      = $(shell lhapdf-config --prefix)

# ROOT installation
#ROOTSYS        = $(shell root-config --prefix)


ifeq ($(HEPMC3SYS),)
$(error Please set HEPMC3SYS environment variable and paths [perhaps 'source ./install/setenv.sh'])
else
$(info Found HEPMC3SYS = $(HEPMC3SYS))
endif

ifeq ($(LHAPDFSYS),)
$(error Please set LHAPDFSYS environment variable and paths [perhaps 'source ./install/setenv.sh'])
else
$(info Found LHAPDFSYS = $(LHAPDFSYS))
endif

# If ROOT seems to be installed, tag ROOT dependent compilation on
ifeq ($(ROOTSYS),)
$(info Did not find ROOTSYS - compilation without ROOT <root.cern.ch> dependent tools)
ROOT=FALSE
else
$(info Found ROOTSYS = $(ROOTSYS))
ROOT=TRUE

# Info messages
$(info )
$(info ************************************************************************************)
$(info **                                                                                **)
$(info **  If compilation with ROOT libraries failes, try with: make -j4 CXX_ROOT=c++17  **)
$(info **                                                                                **)
$(info ************************************************************************************)
$(info )
endif


# =======================================================================
# External libraries to be linked

# HEPMC3  (lib64 needed on some systems)
HEPMC3lib      = -L$(HEPMC3SYS)/lib -L$(HEPMC3SYS)/lib64 -lHepMC3 -lHepMC3search

# LHAPDF6 (lib64 needed on some systems)
LHAPDF6lib     = -L$(LHAPDFSYS)/lib -L$(LHAPDFSYS)/lib64 -lLHAPDF

# PyTorch
# Note -Wl,-rpath-link= handles the recursive dependency (for linker)
#PYTORCHlib     = -L./libs/libtorch/lib -lc10 -ltorch -lgomp -lcaffe2 \
#				 -Wl,-rpath-link=./libs/libtorch/lib

# ROOT
ifeq ($(ROOT),TRUE)
ROOTlib        = -L$(ROOTSYS)/lib -L$(ROOTSYS)/lib/root -lCore -lRIO -lNet \
				 -lHist -lGraf -lGraf3d -lGpad -lTree -lRint \
				 -lPostscript -lMatrix -lPhysics -lMathCore \
				 -lThread -lGui -lRooFit -lMinuit
endif

# C++ standard
STANDARDlib    = -lstdc++ -lm -pthread -lrt
#  -ldl -rdynamic

# GSL, on ubuntu run: sudo apt-get install libgsl-dev
#GSLlib         = -lgsl -lgslcblas


# External libraries (THESE TWO FIRST for priority!!)
LDLIBS  = $(HEPMC3lib)
LDLIBS += $(LHAPDF6lib)

# The rest
LDLIBS += $(STANDARDlib)
#LDLIBS += $(PYTORCHlib)

# BLAS AND LAPACK
#LDLIBS += -llapack -lblas


# =======================================================================
# Header files

# External libraries (THESE TWO FIRST for priority!!)
INCLUDES += -I$(HEPMC3SYS)/include
INCLUDES += -I$(LHAPDFSYS)/include

# C++
INCLUDES += -I/usr/include
INCLUDES += -I/usr/local/include

# Own
INCLUDES += -I.
INCLUDES += -Iinclude

# Internal libraries
INCLUDES += -Ilibs

# FTensor
INCLUDES += -Ilibs/FTensor

# Armadillo
#INCLUDES += -Ilibs/armadillo

# Ensmallen
#INCLUDES += -Ilibs/ensmallen

# Eigen
INCLUDES += -Ilibs/Eigen
INCLUDES += -Ilibs/Eigen/unsupported

# optimlib
#INCLUDES += -Ilibs/optim

# PyTorch
#INCLUDES += -Ilibs/libtorch/include/
#INCLUDES += -Ilibs/libtorch/include/torch/csrc/api/include


# =======================================================================
# Compiler, language version and its options

CXX=g++

# Ubuntu 20.1 GCC 9.3 + pre-compiled ROOT 6.18 for Ubuntu (from root.cern.ch) tested default

ifneq ($(CXX_STANDARD),)
CXXVER     = -std=$(CXX_STANDARD)
else
CXXVER     = -std=c++2a
endif

# Needed for ROOT library
ifneq ($(CXX_ROOT),)
CXXVER_OLD = -std=$(CXX_ROOT)
else
CXXVER_OLD = -std=c++14
endif


OPTIM      = -O3 -DNDEBUG -ftree-vectorize -fno-signed-zeros
CXXFLAGS   = -Wall -fPIC -pipe $(OPTIM)

# Profiling & debug
ifeq ($(VALGRIND),TRUE)
OPTIM   += -pg 
endif


# Needed by PyTorch if using pre-compiled (ABI = Application Binary Interface)
# gcc < 5.1 is 0, later versions use 1 by default
# CXXFLAGS += -D_GLIBCXX_USE_CXX11_ABI=0

# Automatic dependencies on
CXXFLAGS += -MMD -MP 


# -Wall,             compiler warnings full on
# -free-vectorize,   Autovectorization on
# -fno-signed-zeros  Optimization (floating point) which ignore the signedness of zero
# -fPIC,             Position independent code (PIC) for shared libraries
# -O2, -O3           Optimization level
# -DNDEBUG           Release flag, no debug
# -pipe,             Faster compilation using pipes
# -O0 -pg,           Profiling and debugging (HUGE PERFORMANCE HIT)
# -rpath-link        Needed for .so which links to another .so
#
# Fortran like complex number treatments (faster than C++)
# -fcx-fortran-rules -fcx-limited-range
#
# Check your CPU instruction set with: cat /proc/cpuinfo


# DANGEROUS:

# -march=native,     CPU spesific instruction set usage
#  (gives unknown flops problem, factor 1/4 wrong results on i5-4570 with g++7.4, do not use!)
# -ffast-math,       Heavy floating point optimization
#  (fast but breaks IEEE flops standards, do not use!)

# =======================================================================
# Sources, objects and dependency files

OBJ_DIR     = obj



# -----------------------------------------------------------------------
# LIBRARY

## ======================================================================
SRC_DIR_0   = src
SRC_0       = $(wildcard $(SRC_DIR_0)/*.cc)
OBJ_0       = $(SRC_0:$(SRC_DIR_0)/%.cc=$(OBJ_DIR)/%.o)
DEP_0       = $(OBJ_0:$(OBJ_DIR)/%.o=.d)
## ======================================================================

## ======================================================================
SRC_DIR_1   = src/Amplitude
SRC_1       = $(wildcard $(SRC_DIR_1)/*.cc)
OBJ_1       = $(SRC_1:$(SRC_DIR_1)/%.cc=$(OBJ_DIR)/%.o)
DEP_1       = $(OBJ_1:$(OBJ_DIR)/%.o=.d)
## ======================================================================

# Create collection of all library objects
OBJ = $(OBJ_0) $(OBJ_1)

# =======================================================================
ifeq ($(ROOT),TRUE)
SRC_DIR_2   = src/Analysis
SRC_2       = $(wildcard $(SRC_DIR_2)/*.cc)
OBJ_2       = $(SRC_2:$(SRC_DIR_2)/%.cc=$(OBJ_DIR)/%.o)
DEP_2       = $(OBJ_2:$(OBJ_DIR)/%.o=.d)
endif
# =======================================================================

# =======================================================================
ifeq ($(TEST),TRUE)
SRC_DIR_3   = tests/catchlibrary
SRC_3       = $(wildcard $(SRC_DIR_3)/*.cc)
OBJ_3       = $(SRC_3:$(SRC_DIR_3)/%.cc=$(OBJ_DIR)/%.o)
DEP_3       = $(OBJ_3:$(OBJ_DIR)/%.o=.d)
endif
# =======================================================================



# -----------------------------------------------------------------------
# PROGRAM compiled against the library

# =======================================================================
SRC_DIR_PROGRAM      = src/Program
SRC_PROGRAM          = $(wildcard $(SRC_DIR_PROGRAM)/*.cc)
OBJ_PROGRAM          = $(SRC_PROGRAM:$(SRC_DIR_PROGRAM)/%.cc=$(OBJ_DIR)/$(BIN_DIR)/%.o)
DEP_PROGRAM          = $(OBJ_PROGRAM:$(OBJ_DIR)/$(BIN_DIR)/%.o=.d)
# =======================================================================

# =======================================================================
ifeq ($(ROOT),TRUE)
SRC_DIR_PROGRAM_ROOT = src/Program/Analysis
SRC_PROGRAM_ROOT     = $(wildcard $(SRC_DIR_PROGRAM_ROOT)/*.cc)
OBJ_PROGRAM_ROOT     = $(SRC_PROGRAM_ROOT:$(SRC_DIR_PROGRAM_ROOT)/%.cc=$(OBJ_DIR)/$(BIN_DIR)/%.o)
DEP_PROGRAM_ROOT     = $(OBJ_PROGRAM_ROOT:$(OBJ_DIR)/$(BIN_DIR)/%.o=.d)
endif
# =======================================================================

# =======================================================================
ifeq ($(TEST),TRUE)
SRC_DIR_PROGRAM_TEST = tests
SRC_PROGRAM_TEST     = $(wildcard $(SRC_DIR_PROGRAM_TEST)/*.cc)
OBJ_PROGRAM_TEST     = $(SRC_PROGRAM_TEST:$(SRC_DIR_PROGRAM_TEST)/%.cc=$(OBJ_DIR)/$(BIN_DIR)/%.o)
DEP_PROGRAM_TEST     = $(OBJ_PROGRAM_TEST:$(OBJ_DIR)/$(BIN_DIR)/%.o=.d)
endif
# =======================================================================



# -----------------------------------------------------------------------
# PROGRAM

# Directory
BIN_DIR = bin

.SUFFIXES:      .o .cc

# Normal
EXE_NAMES      = gr xscan minbias hepmc3tolhe data2hepmc3 pathmark pdebench sommerfeld ot
PROGRAM        = $(EXE_NAMES:%=$(BIN_DIR)/%)

ifeq ($(ROOT),TRUE)
EXE_ROOT_NAMES = analyze fitsoft fitharmonic
PROGRAM_ROOT   = $(EXE_ROOT_NAMES:%=$(BIN_DIR)/%)
endif

PROGRAM_TEST   =
ifeq ($(TEST),TRUE)
EXE_TEST_NAMES = testbench0 testbench1 testbench2 testbench3
PROGRAM_TEST   = $(EXE_TEST_NAMES:%=$(BIN_DIR)/%)
endif

# Multicore
#export MAKEFLAGS="-j $(grep -c ^processor /proc/cpuinfo)"


# -----------------------------------------------------------------------
# RULES for linking

all: $(PROGRAM) $(PROGRAM_ROOT) $(PROGRAM_TEST)
	@echo " "
	@echo "PROGRAM:" $(PROGRAM) $(PROGRAM_ROOT) $(PROGRAM_TEST)
	@echo " "
	@echo "Compilation of '$@' done."

$(PROGRAM): $(OBJ) $(OBJ_PROGRAM)
	$(CXX) $(OBJ_DIR)/$@.o $(OBJ) -o $@ $(CXXFLAGS) $(LDLIBS)

$(PROGRAM_ROOT): $(OBJ) $(OBJ_2) $(OBJ_PROGRAM_ROOT)
	$(CXX) $(OBJ_DIR)/$@.o $(OBJ) $(OBJ_2) -o $@ $(CXXFLAGS) $(LDLIBS) $(ROOTlib)

# Unit tests (note, we use catchmain.o from $(OBJ_3) for linking with catch2)
$(PROGRAM_TEST): $(OBJ) $(OBJ_3) $(OBJ_PROGRAM_TEST)
	$(CXX) $(OBJ_DIR)/$@.o $(OBJ) $(OBJ_3) -o $@ $(CXXFLAGS) $(LDLIBS)


# -----------------------------------------------------------------------
# RULES to generate library dependencies and compile

# LIBRARY objects
#
# Note that for different ROOT installations, we need both:
# -I$(ROOTSYS)/include
# -I$(ROOTSYS)/include/root

# =======================================================================
$(OBJ_DIR)/%.o: $(SRC_DIR_0)/%.cc
	@echo " "
	@echo "Generating dependencies and compiling $<..."
	$(CXX) $(CXXVER) -c $< -o $@ $(CXXFLAGS) $(INCLUDES)
# =======================================================================

# =======================================================================
$(OBJ_DIR)/%.o: $(SRC_DIR_1)/%.cc
	@echo " "
	@echo "Generating dependencies and compiling $<..."
	$(CXX) $(CXXVER) -c $< -o $@ $(CXXFLAGS) $(INCLUDES)
# =======================================================================

# =======================================================================
$(OBJ_DIR)/%.o: $(SRC_DIR_2)/%.cc
	@echo " "
	@echo "Generating dependencies and compiling $<..."
	$(CXX) $(CXXVER_OLD) -c $< -o $@ $(CXXFLAGS) $(INCLUDES) \
		-I$(ROOTSYS)/include -I$(ROOTSYS)/include/root
# =======================================================================

# =======================================================================
$(OBJ_DIR)/%.o: $(SRC_DIR_3)/%.cc
	@echo " "
	@echo "Generating dependencies and compiling $<..."
	$(CXX) $(CXXVER) -c $< -o $@ $(CXXFLAGS) $(INCLUDES)
# =======================================================================

# PROGRAM objects

# =======================================================================
$(OBJ_DIR)/$(BIN_DIR)/%.o: $(SRC_DIR_PROGRAM)/%.cc
	@echo " "
	@echo "Generating dependencies and compiling $<..."
	$(CXX) $(CXXVER) -c $< -o $@ $(CXXFLAGS) $(INCLUDES)
# =======================================================================

# =======================================================================
$(OBJ_DIR)/$(BIN_DIR)/%.o: $(SRC_DIR_PROGRAM_ROOT)/%.cc
	@echo " "
	@echo "Generating dependencies and compiling $<..."
	$(CXX) $(CXXVER_OLD) -c $< -o $@ $(CXXFLAGS) $(INCLUDES) \
		-I$(ROOTSYS)/include -I$(ROOTSYS)/include/root
# =======================================================================

# =======================================================================
$(OBJ_DIR)/$(BIN_DIR)/%.o: $(SRC_DIR_PROGRAM_TEST)/%.cc
	@echo " "
	@echo "Generating dependencies and compiling $<..."
	$(CXX) $(CXXVER) -c $< -o $@ $(CXXFLAGS) $(INCLUDES)
# =======================================================================

# -----------------------------------------------------------------------
# Clean up of object files
# - ignores return code error
# @ is silent
.PHONY: clean
clean:
	@echo "Cleaning objects"
	-@rm $(OBJ_DIR)/*.o 2>/dev/null || true
	-@rm $(OBJ_DIR)/*.d 2>/dev/null || true
	-@rm $(OBJ_DIR)/$(BIN_DIR)/*.o 2>/dev/null || true
	-@rm $(OBJ_DIR)/$(BIN_DIR)/*.d 2>/dev/null || true


.PHONY: superclean
superclean: clean
	@echo "Cleaning binaries"
	-@rm $(BIN_DIR)/* 2>/dev/null || true

# -----------------------------------------------------------------------
# Dependencies listed here
-include $(DEP_0)
-include $(DEP_1)
ifeq ($(ROOT),TRUE)
-include $(DEP_2)
endif
ifeq ($(TEST),TRUE)
-include $(DEP_3)
endif

# DEBUG printing

#print_vars:
#	echo FOO = ${OBJ}
