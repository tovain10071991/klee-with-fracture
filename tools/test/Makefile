LEVEL=../..
TOOLNAME=test

USEDLIBS = SaibHelper.a FractureCodeInv.a

include $(LEVEL)/Makefile.common

CXX.Flags += -I/home/user/Documents/boost-install/include/boost-1.52.0 -fexceptions

LDFLAGS += -L/home/user/Documents/klee-mcsema-project/mcsema-install/lib/ -L/home/user/Documents/protobuf-2.5.0-install/lib/ -L/home/user/Documents/boost-install/lib/boost-1.52.0/ -L/home/user/Documents/libelf-0.8.13-install/lib/
LLDB_WITHOUT_LLVM_LIBS = -lLLDBWithoutLLVM -lpython2.7 -lpanel -lncurses -lrt
ProjLibsOptions += $(LLDB_WITHOUT_LLVM_LIBS)
LIBS += -lLLVM-3.4 -lelf
