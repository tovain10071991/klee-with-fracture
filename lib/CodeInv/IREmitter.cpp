//===--- IREmitter - Emits IR from SDnodes ----------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class uses SDNodes and emits IR. It is intended to be extended by Target
// implementations who have special ISD legalization nodes.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: October 16, 2013
//===----------------------------------------------------------------------===//

#include "CodeInv/IREmitter.h"
#include "CodeInv/Decompiler.h"

#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Intrinsics.h"
#define GET_REGINFO_ENUM
#include "../lib/Target/X86/X86GenRegisterInfo.inc"
#define GET_INSTRINFO_ENUM
#include "../lib/Target/X86/X86GenInstrInfo.inc"

#include <err.h>
#include <string>

#ifdef DEC_TEST
static unsigned long get_load_addr(unsigned long addr, std::string obj_name, std::string sec_name)
{
  return addr;
}
static std::string get_func_name_in_plt(uint64_t addr)
{
  return "noname";
}
#else
#include "Helper/LLDBHelper.h"
#endif

using namespace llvm;

namespace fracture {

IREmitter::IREmitter(Decompiler *TheDec, raw_ostream &InfoOut,
  raw_ostream &ErrOut) : Infos(InfoOut), Errs(ErrOut) {
  Dec = TheDec;
  IRB = new IRBuilder<>(getGlobalContext());
  initDispatcher();
}

IREmitter::~IREmitter() {
  delete IRB;
}

void IREmitter::initDispatcher()
{
  #include "MOV/MOV_initDispatcher.inc"
  #include "LEA/LEA_initDispatcher.inc"
  
  #include "CMOVcc/CMOVcc_initDispatcher.inc"  
  
  #include "PUSH/PUSH_initDispatcher.inc"
  #include "POP/POP_initDispatcher.inc"
  #include "LEAVE/LEAVE_initDispatcher.inc"
  
  #include "ADD/ADD_initDispatcher.inc"
  #include "SUB/SUB_initDispatcher.inc"

  #include "INC/INC_initDispatcher.inc"
  #include "DEC/DEC_initDispatcher.inc"

  #include "SAR/SAR_initDispatcher.inc"
  #include "SHR/SHR_initDispatcher.inc"
  #include "SHL/SHL_initDispatcher.inc"
  
  #include "AND/AND_initDispatcher.inc"
  #include "OR/OR_initDispatcher.inc"
  #include "XOR/XOR_initDispatcher.inc"
  #include "NEG/NEG_initDispatcher.inc"
    
  #include "CMP/CMP_initDispatcher.inc"
  #include "TEST/TEST_initDispatcher.inc"
  
  #include "JMP/JMP_initDispatcher.inc"
  #include "Jcc/Jcc_initDispatcher.inc"
  
  #include "CALL/CALL_initDispatcher.inc"
  #include "RET/RET_initDispatcher.inc"
  
  #include "REP/REP_initDispatcher.inc"

  #include "STOS/STOS_initDispatcher.inc"

  #include "Misc/Misc_initDispatcher.inc"

  #include "SSE/SSE_initDispatcher.inc"
  
  #include "SYSCALL/SYSCALL_initDispatcher.inc"
  
  #include "NOOP/NOOP_initDispatcher.inc"
  
  #include "Other/Other_initDispatcher.inc"  
}

void IREmitter::EmitIR(BasicBlock *BB, MachineInstr* CurInst) {
  visit(BB, CurInst);
}

StringRef IREmitter::getIndexedValueName(StringRef BaseName) {
  const ValueSymbolTable &ST = Dec->getModule()->getValueSymbolTable();

  // In the common case, the name is not already in the symbol table.
  Value *V = ST.lookup(BaseName);
  if (V == NULL) {
    return BaseName;
  }

  // Otherwise, there is a naming conflict.  Rename this value.
  // FIXME: AFAIK this is never deallocated (memory leak). It should be free'd
  // after it gets added to the symbol table (which appears to do a copy as
  // indicated by the original code that stack allocated this variable).
  SmallString<256> *UniqueName =
    new SmallString<256>(BaseName.begin(), BaseName.end());
  unsigned Size = BaseName.size();

  // Add '_' as the last character when BaseName ends in a number
  if (BaseName[Size-1] <= '9' && BaseName[Size-1] >= '0') {
    UniqueName->resize(Size+1);
    (*UniqueName)[Size] = '_';
    Size++;
  }

  unsigned LastUnique = 0;
  while (1) {
    // Trim any suffix off and append the next number.
    UniqueName->resize(Size);
    raw_svector_ostream(*UniqueName) << ++LastUnique;

    // Try insert the vmap entry with this suffix.
    V = ST.lookup(*UniqueName);
    // FIXME: ^^ this lookup does not appear to be working on non-globals...
    // Temporary Fix: check if it has a basenames entry
    if (V == NULL && BaseNames[*UniqueName].empty()) {
      BaseNames[*UniqueName] = BaseName;
      return *UniqueName;
    }
  }
}

StringRef IREmitter::getBaseValueName(StringRef BaseName) {
  // Note: An alternate approach would be to pull the Symbol table and
  // do a string search, but this is much easier to implement.
  StringRef Res = BaseNames.lookup(BaseName);
  if (Res.empty()) {
    return BaseName;
  }
  return Res;
}

StringRef IREmitter::getInstructionName(MachineInstr *I) {
  return StringRef();
}

void IREmitter::visit(BasicBlock *BB, MachineInstr* CurInst) {
  CurInst->dump();
  for(unsigned i = 0; i < CurInst->getNumOperands(); ++i)
  {
    errs() << "\n\t" << i << ": ";
    CurInst->getOperand(i).print(errs());
  }
  errs() << "\n";

  IRB->SetCurrentDebugLocation(CurInst->getDebugLoc());
  
  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();
  ConstantInt* next_rip = ConstantInt::get(Type::getInt64Ty(*context), get_load_addr(Dec->getDisassembler()->getDebugOffset(CurInst->getDebugLoc()), Dec->getDisassembler()->getExecutable()->getFileName(), Dec->getDisassembler()->getCurrentSectionName()) + CurInst->getDesc().getSize());
  store_reg_val(X86::RIP, next_rip);
  
  assert(visitDispatchers.find(CurInst->getOpcode()) != visitDispatchers.end() && "unknown opcode when decompileBasicBlock");
  void(IREmitter::*dispatcher)(BasicBlock *, MachineInstr*) = visitDispatchers[CurInst->getOpcode()];
  (this->*dispatcher)(BB, CurInst);

  BB->dump();
}

#include "IREmitter_common.inc"

#include "MOV/MOV_define.inc"
#include "LEA/LEA_define.inc"

#include "CMOVcc/CMOVcc_define.inc"

#include "PUSH/PUSH_define.inc"
#include "POP/POP_define.inc"
#include "LEAVE/LEAVE_define.inc"

#include "ADD/ADD_define.inc"
#include "SUB/SUB_define.inc"

#include "INC/INC_define.inc"
#include "DEC/DEC_define.inc"

#include "SAR/SAR_define.inc"
#include "SHR/SHR_define.inc"
#include "SHL/SHL_define.inc"

#include "AND/AND_define.inc"
#include "OR/OR_define.inc"
#include "XOR/XOR_define.inc"
#include "NEG/NEG_define.inc"

#include "CMP/CMP_define.inc"
#include "TEST/TEST_define.inc"

#include "JMP/JMP_define.inc"
#include "Jcc/Jcc_define.inc"

#include "CALL/CALL_define.inc"
#include "RET/RET_define.inc"

#include "REP/REP_define.inc"

#include "STOS/STOS_define.inc"

#include "Misc/Misc_define.inc"

#include "SSE/SSE_define.inc"

#include "SYSCALL/SYSCALL_define.inc"

#include "NOOP/NOOP_define.inc"

} // End namespace fracture
