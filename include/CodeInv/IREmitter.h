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
  std::map<unsigned, void(IREmitter::*)(BasicBlock *, MachineInstr*)> visitDispachers;
  Decompiler *Dec;
  IRBuilder<> *IRB;
  StringMap<StringRef> BaseNames;

  void initDispacher();

  // Visit Functions (Convert SDNode into Instruction/Value)
  virtual void visit(BasicBlock *BB, MachineInstr* CurInst);
#define declare_visit(name) \
  void visit##name(BasicBlock *BB, MachineInstr* I)

  declare_visit(MOV32r);
  declare_visit(MOV32rm);
  declare_visit(MOV64r);
  declare_visit(MOV64ri32);
  declare_visit(MOV8m);
  declare_visit(MOV32m);
  declare_visit(MOV64m);
  declare_visit(MOV64mi32);
  declare_visit(MOV64rm);

  declare_visit(LEA64r);

  declare_visit(PUSH64r);
  declare_visit(POP64r);
  declare_visit(LEAVE64);

  void ADDr(BasicBlock *BB, MachineInstr* I, unsigned init_size, unsigned final_size);
  declare_visit(ADD32rr);
  declare_visit(ADD64rr);
  declare_visit(ADD32ri8);
  declare_visit(ADD64ri8);
  declare_visit(ADD64ri32);
  declare_visit(ADD64i32);

  declare_visit(SUB64i32);
  declare_visit(SUB64ri8);
  declare_visit(SUB64r);

  declare_visit(SAR64r1);
  declare_visit(SAR64ri);
  declare_visit(SHR64ri);

  declare_visit(AND64ri8);
  declare_visit(AND32i32);
  declare_visit(OR64ri8);
  declare_visit(XOR32r);

  declare_visit(NEG32r);

  declare_visit(CMP32ri8);
  declare_visit(CMP64ri8);
  declare_visit(CMP64i32);
  declare_visit(CMP64r);
  declare_visit(CMP32mi8);
  declare_visit(CMP64mi8);
  declare_visit(CMP8mi);
  declare_visit(CMP64rm);

  void Testrr(BasicBlock *BB, MachineInstr* I);
  declare_visit(TEST32rr);
  declare_visit(TEST64rr);

  declare_visit(JMP64r);
  declare_visit(JMP);
  
  void Jcc(BasicBlock *BB, MachineInstr* I, Value* cond_val, int64_t off);
  declare_visit(JA_1);
  declare_visit(JAE_1);
  declare_visit(JBE_1);
  declare_visit(JB_1);
  declare_visit(JE_1);
  declare_visit(JNE_1);
  declare_visit(JG_1);
  declare_visit(JGE_1);
  declare_visit(JL_1);
  declare_visit(JLE_1);
  declare_visit(JNO_1);
  declare_visit(JNP_1);
  declare_visit(JNS_1);
  declare_visit(JO_1);
  declare_visit(JP_1);
  declare_visit(JS_1);

  declare_visit(CALL64pcrel32);
  declare_visit(CALL64r);
  declare_visit(CALL64m);
  declare_visit(RET);

  declare_visit(NOOP);
  
  declare_visit(SYSCALL);
  
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
  Value* get_pointer_val(BasicBlock* BB, unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg);
  Value* get_mem_val(BasicBlock* BB, unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg, unsigned mem_size);
  void store_mem_val(BasicBlock* BB, unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg, Value* val);
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

};

#define define_visit(name) \
void IREmitter::visit##name(BasicBlock *BB, MachineInstr* I)

} // end namespace fracture

#endif /* IREMITTER_H */
