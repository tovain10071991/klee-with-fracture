#===-- tools/klee/Makefile ---------------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..
TOOLNAME = saib

include $(LEVEL)/Makefile.config

USEDLIBS = SaibHelper.a kleeCore.a kleeBasic.a kleeModule.a kleaverSolver.a kleaverExpr.a kleeSupport.a FractureCodeInv.a SaibHelper.a
#LINK_COMPONENTS = jit bitreader bitwriter ipo linker engine
#
#LINK_COMPONENTS += mcjit all-targets mcdisassembler instrumentation support option irreader
#
#ifeq ($(shell python -c "print($(LLVM_VERSION_MAJOR).$(LLVM_VERSION_MINOR) >= 3.3)"), True)
#LINK_COMPONENTS += irreader
#endif
include $(LEVEL)/Makefile.common

ifneq ($(ENABLE_STP),0)
  LIBS += $(STP_LDFLAGS)
endif

ifneq ($(ENABLE_Z3),0)
  LIBS += $(Z3_LDFLAGS)
endif

include $(PROJ_SRC_ROOT)/MetaSMT.mk

ifeq ($(HAVE_TCMALLOC),1)
  LIBS += $(TCMALLOC_LIB)
endif

LDFLAGS += -L/home/user/Documents/klee-mcsema-project/mcsema-install/lib/ -L/home/user/Documents/protobuf-2.5.0-install/lib/ -L/home/user/Documents/boost-install/lib/boost-1.52.0/ -L/home/user/Documents/libelf-0.8.13-install/lib/
LLDB_WITHOUT_LLVM_LIBS = -lLLDBWithoutLLVM -lpython2.7 -lpanel -lncurses -lrt
ProjLibsOptions += $(LLDB_WITHOUT_LLVM_LIBS)
LIBS += -lLLVM-3.4 -lelf