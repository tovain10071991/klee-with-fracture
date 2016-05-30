//===--- Decompiler - Decompiles machine basic blocks -----------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class uses the disassembler and inverse instruction selector classes
// to decompile target specific code into LLVM IR and return function objects
// back to the user.
//
//===----------------------------------------------------------------------===//

#ifndef DECOMPILER_H
#define DECOMPILER_H
/*
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/PassManager.h"
*/
#include "CodeInv/Disassembler.h"
#include "CodeInv/IREmitter.h"

#include <map>

using namespace llvm;

namespace fracture {

class Decompiler {
public:

  Decompiler(Disassembler *NewDis, Module *NewMod = NULL,
    raw_ostream &InfoOut = nulls(), raw_ostream &ErrOut = nulls());
  ~Decompiler();

  void printInstructions(formatted_raw_ostream &Out, unsigned long Address);

  ///===-------------------------------------------------------------------===//
  /// decompile - decompile starting at a given memory address.
  ///
  /// This function recursively descends the code to decompile the function
  /// and all functions called by this function.
  ///
  /// Results are cached, so if a function has already been decompiled, we refer
  /// to the stored result.
  ///
  /// @param Address - the address to start decompiling.
  ///
  void decompile(unsigned long Address);
  Function* decompileFunction(unsigned long Address);
  BasicBlock* decompileBasicBlock(MachineBasicBlock *MBB, Function *F);

  BasicBlock* getOrCreateBasicBlock(unsigned long Address, Function *F);
  BasicBlock* getOrCreateBasicBlock(StringRef BBName, Function *F);

  void sortBasicBlock(BasicBlock *BB);
  void splitBasicBlockIntoBlock(Function::iterator Src,
    BasicBlock::iterator FirstInst, BasicBlock *Tgt);

  uint64_t getBasicBlockAddress(BasicBlock *BB);

  const Disassembler* getDisassembler() const { return Dis; }
  Module* getModule() { return Mod; }
  LLVMContext *getContext() {return Context;}

  Function* getFunctionByAddr(uint64_t addr);
private:
  Disassembler *Dis;
  Module *Mod;
  LLVMContext *Context;
  IREmitter *Emitter;

  /// Error printing
  raw_ostream &Infos, &Errs;
  void printInfo(std::string Msg) const {
    Infos << "Disassembler: " << Msg << "\n";
  }
  void printError(std::string Msg) const {
    Errs << "Disassembler: " << Msg << "\n";
    Errs.flush();
  }
  void initModule();
};

} // end namespace fracture

#endif /* DECOMPILER_H */
