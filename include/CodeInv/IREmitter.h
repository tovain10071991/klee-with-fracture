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

#ifndef IREMITTER_H
#define IREMITTER_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringMap.h"

#include <map>

using namespace llvm;

namespace fracture {

class Decompiler;

class IREmitter {
public:
  IREmitter(Decompiler *TheDec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  virtual ~IREmitter();

  void EmitIR(BasicBlock *BB, MachineInstr* CurInst);

  // This function emulates createValueName(StringRef Name, Value *V) in the
  // ValueSymbolTable class, with exception that BaseName's ending in a number
  // get an additional "_" added to the end.
  StringRef getIndexedValueName(StringRef BaseName);
  // Returns the BaseName used to create the given indexed value name, or the
  // same name if it doesn't exist.
  StringRef getBaseValueName(StringRef BaseName);
  // Returns the Instruction Name by looking for a CopyToReg as a node user, or
  // returns empty.
  StringRef getInstructionName(MachineInstr* I);


  IRBuilder<>* getIRB() { return IRB; }

protected:
  std::map<unsigned, void(IREmitter::*)(BasicBlock *, MachineInstr*)> visitDispatchers;
  Decompiler *Dec;
  IRBuilder<> *IRB;
  StringMap<StringRef> BaseNames;

  void initDispatcher();

  // Visit Functions (Convert SDNode into Instruction/Value)
  virtual void visit(BasicBlock *BB, MachineInstr* CurInst);
#define declare_visit(name) \
  void visit##name(BasicBlock *BB, MachineInstr* I)

  #include "../lib/CodeInv/MOV/MOV_declare.inc"
  #include "../lib/CodeInv/LEA/LEA_declare.inc"

  #include "../lib/CodeInv/CMOVcc/CMOVcc_declare.inc"

  #include "../lib/CodeInv/PUSH/PUSH_declare.inc"
  #include "../lib/CodeInv/POP/POP_declare.inc"
  #include "../lib/CodeInv/LEAVE/LEAVE_declare.inc"

  #include "../lib/CodeInv/ADD/ADD_declare.inc"
  #include "../lib/CodeInv/SUB/SUB_declare.inc"

  #include "../lib/CodeInv/SAR/SAR_declare.inc"
  #include "../lib/CodeInv/SHR/SHR_declare.inc"

  #include "../lib/CodeInv/AND/AND_declare.inc"
  #include "../lib/CodeInv/OR/OR_declare.inc"
  #include "../lib/CodeInv/XOR/XOR_declare.inc"
  #include "../lib/CodeInv/NEG/NEG_declare.inc"

  #include "../lib/CodeInv/CMP/CMP_declare.inc"
  #include "../lib/CodeInv/TEST/TEST_declare.inc"
  
  #include "../lib/CodeInv/JMP/JMP_declare.inc"
  #include "../lib/CodeInv/Jcc/Jcc_declare.inc"

  #include "../lib/CodeInv/CALL/CALL_declare.inc"
  #include "../lib/CodeInv/RET/RET_declare.inc"

  #include "../lib/CodeInv/REP/REP_declare.inc"
  
  #include "../lib/CodeInv/STOS/STOS_declare.inc"  

  #include "../lib/CodeInv/SYSCALL/SYSCALL_declare.inc"

  #include "../lib/CodeInv/NOOP/NOOP_declare.inc"
    
  /// Error printing
  raw_ostream &Infos, &Errs;
  void printInfo(std::string Msg) const {
    Infos << "IREmitter: " << Msg << "\n";
  }
  void printError(std::string Msg) const {
    Errs << "IREmitter: " << Msg << "\n";
    Errs.flush();
  }

  /// helper function
  unsigned get_super_reg(unsigned reg);
  Value* get_reg_val(unsigned reg);
  void store_reg_val(unsigned reg, Value* val);
  Value* get_pointer_val(unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg);
  Value* get_mem_val(unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg, unsigned mem_size);
  void store_mem_val(unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg, Value* val);
  Constant* get_imm_val(int64_t imm, unsigned init_size, unsigned final_size);

#define declare_store_flag_val(name) \
  void store_##name##_val(int opcode, Value* lhs_val, Value* rhs_val, Value* result)
  declare_store_flag_val(AF);
  declare_store_flag_val(PF);
  declare_store_flag_val(ZF);
  declare_store_flag_val(SF);
  declare_store_flag_val(CF);
  declare_store_flag_val(OF);

#define declare_get_flag_val(name) \
  Value* get_##name##_val()
  declare_get_flag_val(AF);
  declare_get_flag_val(PF);
  declare_get_flag_val(ZF);
  declare_get_flag_val(SF);
  declare_get_flag_val(CF);
  declare_get_flag_val(OF);
  declare_get_flag_val(DF);

};

#define define_visit(name) \
void IREmitter::visit##name(BasicBlock *BB, MachineInstr* I)

} // end namespace fracture

#endif /* IREMITTER_H */
