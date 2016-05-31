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

#include "Helper/LLDBHelper.h"

using namespace llvm;

namespace fracture {

IREmitter::IREmitter(Decompiler *TheDec, raw_ostream &InfoOut,
  raw_ostream &ErrOut) : Infos(InfoOut), Errs(ErrOut) {
  Dec = TheDec;
  IRB = new IRBuilder<>(getGlobalContext());
  initDispacher();
}

IREmitter::~IREmitter() {
  delete IRB;
}

void IREmitter::initDispacher()
{
  visitDispachers[X86::MOV32ri] = &IREmitter::visitMOV32r;
  visitDispachers[X86::MOV32rr] = &IREmitter::visitMOV32r;
  visitDispachers[X86::MOV32rm] = &IREmitter::visitMOV32rm;
  visitDispachers[X86::MOV64rr] = &IREmitter::visitMOV64r;
  visitDispachers[X86::MOV64ri32] = &IREmitter::visitMOV64ri32;
  visitDispachers[X86::MOV8mi] = &IREmitter::visitMOV8m;
  visitDispachers[X86::MOV32mi] = &IREmitter::visitMOV32m;
  visitDispachers[X86::MOV32mr] = &IREmitter::visitMOV32m;
  visitDispachers[X86::MOV64mr] = &IREmitter::visitMOV64m;
  visitDispachers[X86::MOV64mi32] = &IREmitter::visitMOV64mi32;
  visitDispachers[X86::MOV64rm] = &IREmitter::visitMOV64rm;
  
  visitDispachers[X86::LEA64r] = &IREmitter::visitLEA64r;
  
  visitDispachers[X86::PUSH64r] = &IREmitter::visitPUSH64r;
  visitDispachers[X86::POP64r] = &IREmitter::visitPOP64r;
  
  visitDispachers[X86::ADD64rr] = &IREmitter::visitADD64r;
  visitDispachers[X86::ADD32ri8] = &IREmitter::visitADD32ri8;
  visitDispachers[X86::ADD64ri8] = &IREmitter::visitADD64ri8;
  
  visitDispachers[X86::SUB64i32] = &IREmitter::visitSUB64i32;
  visitDispachers[X86::SUB64ri8] = &IREmitter::visitSUB64ri8;
  visitDispachers[X86::SUB64rr] = &IREmitter::visitSUB64r;
  
  visitDispachers[X86::SAR64r1] = &IREmitter::visitSAR64r1;
  visitDispachers[X86::SAR64ri] = &IREmitter::visitSAR64ri;
  visitDispachers[X86::SHR64ri] = &IREmitter::visitSHR64ri;
  
  visitDispachers[X86::AND64ri8] = &IREmitter::visitAND64ri8;
  visitDispachers[X86::XOR32rr] = &IREmitter::visitXOR32r;
  
  visitDispachers[X86::CMP32ri8] = &IREmitter::visitCMP32ri8;
  visitDispachers[X86::CMP64ri8] = &IREmitter::visitCMP64ri8;
  visitDispachers[X86::CMP64rr] = &IREmitter::visitCMP64r;
  visitDispachers[X86::CMP32mi8] = &IREmitter::visitCMP32mi8;
  visitDispachers[X86::CMP64mi8] = &IREmitter::visitCMP64mi8;
  visitDispachers[X86::CMP8mi] = &IREmitter::visitCMP8mi;
  
  visitDispachers[X86::TEST32rr] = &IREmitter::visitTEST32rr;
  visitDispachers[X86::TEST64rr] = &IREmitter::visitTEST64rr;
  
  visitDispachers[X86::JMP64r] = &IREmitter::visitJMP64r;
  visitDispachers[X86::JMP_1] = &IREmitter::visitJMP;
  visitDispachers[X86::JMP64pcrel32] = &IREmitter::visitJMP;
  visitDispachers[X86::JA_1] = &IREmitter::visitJA_1;
  visitDispachers[X86::JE_1] = &IREmitter::visitJE_1;
  visitDispachers[X86::JNE_1] = &IREmitter::visitJNE_1;
  
  visitDispachers[X86::CALL64pcrel32] = &IREmitter::visitCALL64pcrel32;
  visitDispachers[X86::CALL64r] = &IREmitter::visitCALL64r;
  visitDispachers[X86::CALL64m] = &IREmitter::visitCALL64m;
  visitDispachers[X86::RET] = &IREmitter::visitRET;
  visitDispachers[X86::LEAVE64] = &IREmitter::visitLEAVE64;
  
  visitDispachers[X86::NOOP] = &IREmitter::visitNOOP;
  visitDispachers[X86::NOOPL] = &IREmitter::visitNOOP;
  visitDispachers[X86::NOOPW] = &IREmitter::visitNOOP;
  visitDispachers[X86::REP_PREFIX] = &IREmitter::visitNOOP;
  visitDispachers[X86::HLT] = &IREmitter::visitNOOP;
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
  assert(visitDispachers.find(CurInst->getOpcode()) != visitDispachers.end() && "unknown opcode when decompileBasicBlock");
  void(IREmitter::*dispatcher)(BasicBlock *, MachineInstr*) = visitDispachers[CurInst->getOpcode()];
  (this->*dispatcher)(BB, CurInst);

  BB->dump();
}

unsigned IREmitter::get_super_reg(unsigned reg)
{
  const MCRegisterInfo* MRI = Dec->getDisassembler()->getMCDirector()->getMCRegisterInfo();

  MCSuperRegIterator super_reg_iter(reg, MRI);
  for(; super_reg_iter.isValid(); ++super_reg_iter)
  {
   if(!MCSuperRegIterator(*super_reg_iter, MRI).isValid())
    {
      return *super_reg_iter;
    }
  }
  return reg;
}

Value* IREmitter::get_reg_val(unsigned reg)
{
  const MCRegisterInfo* MRI = Dec->getDisassembler()->getMCDirector()->getMCRegisterInfo();
  LLVMContext* context = Dec->getContext();

  unsigned super_reg = get_super_reg(reg);
  Type* reg_type = Dec->getDisassembler()->getMCDirector()->getRegType(reg).getTypeForEVT(*context);
  ConstantInt* offset_const = reg==super_reg ? ConstantInt::get(Type::getInt64Ty(*context), 0) : ConstantInt::get(Type::getInt64Ty(*context), MRI->getSubRegIdxOffset(MRI->getSubRegIndex(super_reg, reg)));

  GlobalVariable* super_reg_var = Dec->getModule()->getGlobalVariable(MRI->getName(super_reg));
  assert(super_reg_var && "can not find src super reg var");
  Value* val = IRB->CreateLoad(super_reg_var);

  if(reg != super_reg)
  {
    if(!offset_const->equalsInt(0))
      val = IRB->CreateLShr(val, offset_const);
    val = IRB->CreateTrunc(val, reg_type);
  }
  return val;
}

void IREmitter::store_reg_val(unsigned reg, Value* val)
{
  const MCRegisterInfo* MRI = Dec->getDisassembler()->getMCDirector()->getMCRegisterInfo();
  LLVMContext* context = Dec->getContext();

  unsigned super_reg = get_super_reg(reg);
  Type* reg_type = Dec->getDisassembler()->getMCDirector()->getRegType(reg).getTypeForEVT(*context);
  Type* super_reg_type = Dec->getDisassembler()->getMCDirector()->getRegType(super_reg).getTypeForEVT(*context);
  ConstantInt* offset_const = reg==super_reg ? ConstantInt::get(Type::getInt64Ty(*context), 0) : ConstantInt::get(Type::getInt64Ty(*context), MRI->getSubRegIdxOffset(MRI->getSubRegIndex(super_reg, reg)));

  GlobalVariable* super_reg_var = Dec->getModule()->getGlobalVariable(MRI->getName(super_reg));
  assert(super_reg_var && "can not find des super reg var");

  if(reg!=super_reg)
  {
    Constant* mask = ConstantExpr::getNot(ConstantExpr::getShl(ConstantExpr::getZExt(ConstantInt::getAllOnesValue(reg_type), super_reg_type), offset_const));
    
    Value* result_val = IRB->CreateLoad(super_reg_var);

    val = IRB->CreateZExt(val, super_reg_type);
    if(!offset_const->equalsInt(0))
      val = IRB->CreateShl(val, offset_const);
    val = IRB->CreateOr(val, IRB->CreateAnd(result_val, mask));
  }

  IRB->CreateStore(val, super_reg_var);
}

Value* IREmitter::get_pointer_val(unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg)
{
  LLVMContext* context = Dec->getContext();

  Value* base_val = base_reg==X86::NoRegister ? ConstantInt::get(Type::getInt64Ty(*context), 0) : get_reg_val(base_reg);
  Value* scale_val = ConstantInt::get(Type::getInt64Ty(*context), scale);
  Value* idx_val = idx_reg==X86::NoRegister ? ConstantInt::get(Type::getInt64Ty(*context), 0) : get_reg_val(idx_reg);
  Value* offset_val = ConstantInt::get(Type::getInt64Ty(*context), offset);
  assert(seg_reg==X86::NoRegister && "seg reg is not noreg");

  return IRB->CreateAdd(base_val, IRB->CreateAdd(offset_val, IRB->CreateMul(idx_val, scale_val)));
}

Value* IREmitter::get_mem_val(unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg, unsigned mem_size)
{
  LLVMContext* context = Dec->getContext();

  Value* pointer = IRB->CreateIntToPtr(get_pointer_val(base_reg, scale, idx_reg, offset, seg_reg), PointerType::get(Type::getIntNTy(*context, mem_size), 0));

  return IRB->CreateLoad(pointer);
}

void IREmitter::store_mem_val(unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg, Value* val)
{
  Value* pointer = IRB->CreateIntToPtr(get_pointer_val(base_reg, scale, idx_reg, offset, seg_reg), PointerType::get(val->getType(), 0));

  IRB->CreateStore(val, pointer);
}

Constant* IREmitter::get_imm_val(int64_t imm, unsigned init_size, unsigned final_size)
{
  LLVMContext* context = Dec->getContext();

  Constant* imm_val = ConstantInt::get(Type::getIntNTy(*context, init_size), imm);
  assert((init_size < final_size || init_size == final_size) && "init_size > final_size");
  if(init_size<final_size)
    imm_val = ConstantExpr::getSExt(imm_val, Type::getIntNTy(*context, final_size));
  return imm_val;
}

#define compute_AF \
  IRB->CreateICmpNE(IRB->CreateAnd(IRB->CreateXor(IRB->CreateXor(result, lhs_val), rhs_val), 16), ConstantInt::get(result->getType(), 0))

#define compute_PF \
({ \
  Value* src = IRB->CreateTrunc(result, Type::getInt8Ty(*Dec->getContext())); \
  Value* res = IRB->CreateAnd(src, 1); \
  Value* tmp = IRB->CreateAnd(IRB->CreateLShr(src, 1), 1); \
  res = IRB->CreateAdd(res, tmp); \
  tmp = IRB->CreateAnd(IRB->CreateLShr(src, 2), 1); \
  res = IRB->CreateAdd(res, tmp); \
  tmp = IRB->CreateAnd(IRB->CreateLShr(src, 3), 1); \
  res = IRB->CreateAdd(res, tmp); \
  tmp = IRB->CreateAnd(IRB->CreateLShr(src, 4), 1); \
  res = IRB->CreateAdd(res, tmp); \
  tmp = IRB->CreateAnd(IRB->CreateLShr(src, 5), 1); \
  res = IRB->CreateAdd(res, tmp); \
  tmp = IRB->CreateAnd(IRB->CreateLShr(src, 6), 1); \
  res = IRB->CreateAdd(res, tmp); \
  tmp = IRB->CreateAnd(IRB->CreateLShr(src, 7), 1); \
  res = IRB->CreateAdd(res, tmp); \
  res = IRB->CreateXor(IRB->CreateTrunc(res, Type::getInt1Ty(*Dec->getContext())), ConstantInt::getTrue(*Dec->getContext())); \
})

#define compute_ZF \
  IRB->CreateICmpEQ(result, ConstantInt::get(result->getType(), 0))

#define compute_SF \
  IRB->CreateTrunc(IRB->CreateLShr(result, dyn_cast<IntegerType>(result->getType())->getBitWidth()-1), Type::getInt1Ty(*Dec->getContext()))

#define compute_CF \
  IRB->CreateICmpULT(lhs_val, rhs_val)

#define compute_OF \
  IRB->CreateTrunc(IRB->CreateLShr(IRB->CreateAnd(IRB->CreateXor(lhs_val, rhs_val), IRB->CreateXor(lhs_val, result)), dyn_cast<IntegerType>(result->getType())->getBitWidth()-1), Type::getInt1Ty(*Dec->getContext()))

#define define_store_flag_val(name) \
void IREmitter::store_##name##_val(int opcode, Value* lhs_val, Value* rhs_val, Value* result) \
{ \
  IRB->CreateStore(compute_##name, Dec->getModule()->getGlobalVariable(#name)); \
}

define_store_flag_val(AF)
define_store_flag_val(PF)
define_store_flag_val(ZF)
define_store_flag_val(SF)
define_store_flag_val(CF)
define_store_flag_val(OF)

#define define_get_flag_val(name) \
Value* IREmitter::get_##name##_val() \
{ \
  std::string flag_name = #name; \
\
  GlobalVariable* flag_var = Dec->getModule()->getGlobalVariable(flag_name); \
  assert(flag_var && "can not find flag"); \
  return IRB->CreateLoad(flag_var); \
\
}

define_get_flag_val(AF)
define_get_flag_val(PF)
define_get_flag_val(ZF)
define_get_flag_val(SF)
define_get_flag_val(CF)
define_get_flag_val(OF)

#define define_visit(name) \
void IREmitter::visit##name(BasicBlock *BB, MachineInstr* I)

define_visit(MOV32r)
{
  assert(I->getNumOperands()==2 && "MOV's opr's num is not 2");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && "opr 0(dest) is not reg in IREmitter::visitMOVr");
  MachineOperand& src_opr = I->getOperand(1);

  IRB->SetInsertPoint(BB);

  // read src val
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 32, 32);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_visit(MOV32rm)
{
  assert(I->getNumOperands()==6);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg());
  MachineOperand& base_opr = I->getOperand(1);
  assert(base_opr.isReg());
  MachineOperand& scale_opr = I->getOperand(2);
  assert(scale_opr.isImm());
  MachineOperand& idx_opr = I->getOperand(3);
  assert(idx_opr.isReg());
  MachineOperand& off_opr = I->getOperand(4);
  assert(off_opr.isImm());
  MachineOperand& seg_opr = I->getOperand(5);
  assert(seg_opr.isReg());

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), 32);

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_visit(MOV64r)
{
  assert(I->getNumOperands()==2);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg());
  MachineOperand& src_opr = I->getOperand(1);

  IRB->SetInsertPoint(BB);

  // read src val
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 64, 64);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_visit(MOV64ri32)
{
  assert(I->getNumOperands()==2);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg());
  MachineOperand& src_opr = I->getOperand(1);
  assert(src_opr.isImm());

  IRB->SetInsertPoint(BB);

  // read src val
  Value* src_val = get_imm_val(src_opr.getImm(), 32, 64);

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_visit(MOV8m)
{
  assert(I->getNumOperands()==6);
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg());
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm());
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg());
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm());
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg());
  MachineOperand& src_opr = I->getOperand(5);

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 8, 8);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_visit(MOV32m)
{
  assert(I->getNumOperands()==6 && "MOV32mr's opr's num is not 6");
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg() && "opr 0(base) is not reg in IREmitter::visitMOV32mr");
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm() && "opr 1(scale) is not imm in IREmitter::visitMOV32mr");
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg() && "opr 2(idx) is not reg in IREmitter::visitMOV32mr");
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm() && "opr 3(off) is not imm in IREmitter::visitMOV32mr");
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg() && seg_opr.getReg()==X86::NoRegister && "opr 4(seg) is not noreg in IREmitter::visitMOV32mr");
  MachineOperand& src_opr = I->getOperand(5);

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 32, 32);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_visit(MOV64m)
{
  assert(I->getNumOperands()==6 && "MOV64mr's opr's num is not 6");
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg() && "opr 0(base) is not reg in IREmitter::visitMOV64m");
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm() && "opr 1(scale) is not imm in IREmitter::visitMOV64m");
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg() && "opr 2(idx) is not reg in IREmitter::visitMOV64m");
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm() && "opr 3(off) is not imm in IREmitter::visitMOV64m");
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg() && seg_opr.getReg()==X86::NoRegister && "opr 4(seg) is not noreg in IREmitter::visitMOV32mr");
  MachineOperand& src_opr = I->getOperand(5);

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 64, 64);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_visit(MOV64mi32)
{
  assert(I->getNumOperands()==6 && "MOV64mr's opr's num is not 6");
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg() && "opr 0(base) is not reg in IREmitter::visitMOV64m");
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm() && "opr 1(scale) is not imm in IREmitter::visitMOV64m");
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg() && "opr 2(idx) is not reg in IREmitter::visitMOV64m");
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm() && "opr 3(off) is not imm in IREmitter::visitMOV64m");
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg() && seg_opr.getReg()==X86::NoRegister && "opr 4(seg) is not noreg in IREmitter::visitMOV64m");
  MachineOperand& src_opr = I->getOperand(5);
  assert(src_opr.isImm() && "opr 5(imm) is not imm in IREmitter::visitMOV64m");

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val = get_imm_val(src_opr.getImm(), 32, 64);

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_visit(MOV64rm)
{
  assert(I->getNumOperands()==6 && "MOV64mr's opr's num is not 6");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && "opr 0(des) is not reg in IREmitter::visitMOV64rm");
  MachineOperand& base_opr = I->getOperand(1);
  assert(base_opr.isReg() && "opr 1(base) is not reg in IREmitter::visitMOV64rm");
  MachineOperand& scale_opr = I->getOperand(2);
  assert(scale_opr.isImm() && "opr 2(scale) is not imm in IREmitter::visitMOV64rm");
  MachineOperand& idx_opr = I->getOperand(3);
  assert(idx_opr.isReg() && "opr 3(idx) is not reg in IREmitter::visitMOV64rm");
  MachineOperand& off_opr = I->getOperand(4);
  assert(off_opr.isImm() && "opr 4(off) is not imm in IREmitter::visitMOV64rm");
  MachineOperand& seg_opr = I->getOperand(5);
  assert(seg_opr.isReg() && seg_opr.getReg()==X86::NoRegister && "opr 5(seg) is not noreg in IREmitter::visitMOV64rm");

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), 64);

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_visit(LEA64r)
{
  assert(I->getNumOperands()==6 && "LEA64r's opr's num is not 6");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && "opr 0(des) is not reg in IREmitter::visitLEA64r");
  MachineOperand& base_opr = I->getOperand(1);
  assert(base_opr.isReg() && "opr 1(base) is not reg in IREmitter::visitLEA64r");
  MachineOperand& scale_opr = I->getOperand(2);
  assert(scale_opr.isImm() && "opr 2(scale) is not imm in IREmitter::visitLEA64r");
  MachineOperand& idx_opr = I->getOperand(3);
  assert(idx_opr.isReg() && "opr 3(idx) is not reg in IREmitter::visitLEA64r");
  MachineOperand& off_opr = I->getOperand(4);
  assert(off_opr.isImm() && "opr 4(off) is not imm in IREmitter::visitLEA64r");
  MachineOperand& seg_opr = I->getOperand(5);
  assert(seg_opr.isReg() && seg_opr.getReg()==X86::NoRegister && "opr 5(seg) is not noreg in IREmitter::visitLEA64r");

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val = get_pointer_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg());

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_visit(ADD64r)
{
  assert(I->getNumOperands()==4);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val;
  if(rhs_opr.isImm())
  {
    rhs_val = get_imm_val(rhs_opr.getImm(), 64, 64);
  }
  else
  {
    rhs_val = get_reg_val(rhs_opr.getReg());
  }

  // compute
  Value* result = IRB->CreateAdd(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(ADD32ri8)
{
  assert(I->getNumOperands()==4);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(rhs_opr.isImm());
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 32);

  // compute
  Value* result = IRB->CreateAdd(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(ADD64ri8)
{
  assert(I->getNumOperands()==4);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(rhs_opr.isImm());
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 64);

  // compute
  Value* result = IRB->CreateAdd(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(SUB64i32)
{
  assert(I->getNumOperands()==4 && "SUB64i32's opr's num is not 4");
  MachineOperand& rhs_opr = I->getOperand(0);
  assert(rhs_opr.isImm() && "opr 0(rhs imm) is not imm in IREmitter::visitSUB64i32");
  MachineOperand& lhs_opr = I->getOperand(3);
  assert(lhs_opr.isReg() && "opr 3(lhs reg) is not reg in IREmitter::visitSUBNi32");
  MachineOperand& des_opr = I->getOperand(1);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg() && "opr 1(defed reg) is not used reg in IREmitter::visitSUBNi32");
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS && "opr 2(falgs) is not eflags in IREmitter::visitSUBNi32");
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), 32, 64);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(SUB64ri8)
{
  assert(I->getNumOperands()==4 && "SUB64ri8's opr's num is not 4");
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg() && "opr 1(lhs reg) is not reg in IREmitter::visitSUB64ri8");
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(rhs_opr.isImm() && "opr 2(rhs imm) is not imm in IREmitter::visitSUB64ri8");
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS && "opr 3(efalgs) is not eflags in IREmitter::visitSUBNi32");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg() && "opr 0(defed reg) is not used reg in IREmitter::visitSUB64ri8");
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 64);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(SUB64r)
{
  assert(I->getNumOperands()==4);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  //read rhs
  Value* rhs_val;
  if(rhs_opr.isImm())
  {
    rhs_val = get_imm_val(rhs_opr.getImm(), 64, 64);
  }
  else
  {
    rhs_val = get_reg_val(rhs_opr.getReg());
  }

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(SAR64r1)
{
  assert(I->getNumOperands()==3);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(1, 64, 64);

  // compute
  Value* result = IRB->CreateAShr(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(SAR64ri)
{
  assert(I->getNumOperands()==4);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(rhs_opr.isImm());
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), 64, 64);

  // compute
  Value* result = IRB->CreateAShr(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(SHR64ri)
{
  assert(I->getNumOperands()==4);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(rhs_opr.isImm());
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), 64, 64);

  // compute
  Value* result = IRB->CreateLShr(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(AND64ri8)
{
  assert(I->getNumOperands()==4);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(rhs_opr.isImm());
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 64);

  // compute
  Value* result = IRB->CreateAnd(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("CF"));
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("OF"));
}

define_visit(XOR32r)
{
  assert(I->getNumOperands()==4);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());
  
  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  //read rhs
  Value* rhs_val;
  if(rhs_opr.isImm())
  {
    rhs_val = get_imm_val(rhs_opr.getImm(), 32, 32);
  }
  else
  {
    rhs_val = get_reg_val(rhs_opr.getReg());
  }

  // compute
  Value* result = IRB->CreateXor(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("CF"));
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("OF"));
}

define_visit(CMP32ri8)
{
  assert(I->getNumOperands()==3);
  MachineOperand& lhs_opr = I->getOperand(0);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(1);
  assert(rhs_opr.isImm());
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS);
  
  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 32);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(CMP64ri8)
{
  assert(I->getNumOperands()==3);
  MachineOperand& lhs_opr = I->getOperand(0);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(1);
  assert(rhs_opr.isImm());
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS);
  
  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 64);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(CMP64r)
{
  assert(I->getNumOperands()==3);
  MachineOperand& lhs_opr = I->getOperand(0);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(1);
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS);
  
  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val;
  if(rhs_opr.isImm())
  {
    rhs_val = get_imm_val(rhs_opr.getImm(), 64, 64);
  }
  else
  {
    rhs_val = get_reg_val(rhs_opr.getReg());
  }

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(CMP32mi8)
{
  assert(I->getNumOperands()==7 && "CMP32mi8's opr's num is not 7");
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg() && "opr 0(base) is not reg in IREmitter::visitCMP32mi8");
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm() && "opr 1(scale) is not imm in IREmitter::visitCMP32mi8");
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg() && "opr 2(idx) is not reg in IREmitter::visitCMP32mi8");
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm() && "opr 3(off) is not imm in IREmitter::visitCMP32mi8");
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg() && seg_opr.getReg()==X86::NoRegister && "opr 4(seg) is not noreg in IREmitter::visitCMP32mi8");
  MachineOperand& rhs_opr = I->getOperand(5);
  assert(I->getOperand(6).isReg() && I->getOperand(6).getReg()==X86::EFLAGS && "opr 2(efalgs) is not eflags in IREmitter::visitCMP32mi8");

  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), 32);

  //read rhs
  Value* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 32);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(CMP64mi8)
{
  assert(I->getNumOperands()==7);
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg());
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm());
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg());
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm());
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(5);
  assert(I->getOperand(6).isReg() && I->getOperand(6).getReg()==X86::EFLAGS);

  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), 64);

  //read rhs
  Value* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 64);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(CMP8mi)
{
  assert(I->getNumOperands()==7);
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg());
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm());
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg());
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm());
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(5);
  assert(I->getOperand(6).isReg() && I->getOperand(6).getReg()==X86::EFLAGS);

  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), 8);

  //read rhs
  Value* rhs_val = get_imm_val(rhs_opr.getImm(), 8, 8);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(TEST32rr)
{
  assert(I->getNumOperands()==3 && "opr's num is not 3");
  MachineOperand& lhs_opr = I->getOperand(0);
  assert(lhs_opr.isReg() && "opr 0(lhs reg) is not reg");
  MachineOperand& rhs_opr = I->getOperand(1);
  assert(rhs_opr.isReg() && "opr 1(rhs reg) is not reg");
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS && "opr 2(efalgs) is not eflags");
  
  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  // read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val = get_reg_val(rhs_opr.getReg());

  // compute
  Value* result = IRB->CreateAnd(lhs_val, rhs_val);

  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("CF"));
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("OF"));
}

define_visit(TEST64rr)
{
  assert(I->getNumOperands()==3 && "TEST64rr's opr's num is not 3");
  MachineOperand& lhs_opr = I->getOperand(0);
  assert(lhs_opr.isReg() && "opr 0(lhs reg) is not reg in IREmitter::visitTEST64rr");
  MachineOperand& rhs_opr = I->getOperand(1);
  assert(rhs_opr.isReg() && "opr 1(rhs reg) is not reg in IREmitter::visitTEST64rr");
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS && "opr 2(efalgs) is not eflags in IREmitter::visitTEST64rr");
  
  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  // read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val = get_reg_val(rhs_opr.getReg());

  // compute
  Value* result = IRB->CreateAnd(lhs_val, rhs_val);

  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("CF"));
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("OF"));
}

define_visit(PUSH64r)
{
  assert(I->getNumOperands()==3 && "PUSH64r's opr's num is not 3");
  MachineOperand& src_opr = I->getOperand(0);

  IRB->SetInsertPoint(BB);

  // rsp = rsp -8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateSub(lhs_val, rhs_val));

  // mov src, (%rsp)
  // read src val
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 64, 64);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  store_mem_val(X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, result);
}

define_visit(POP64r)
{
  assert(I->getNumOperands()==3 && "POP64r's opr's num is not 3");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && "opr 0(des reg) is not reg in IREmitter::visitPOP64r");

  IRB->SetInsertPoint(BB);

  // mov (%rsp), des
  // read src
  Value* src_val = get_mem_val(X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, 64);

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);

  // rsp = rsp + 8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateAdd(lhs_val, rhs_val));
}

define_visit(LEAVE64)
{
  assert(I->getNumOperands()==4 && "LEAVE64's opr's num is not 4");
  assert(I->getOperand(0).isReg() && I->getOperand(0).getReg()==X86::RBP && "opr 0(rbp reg) is not rbp reg in IREmitter::visitLEAVE64");
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::RSP && "opr 1(rsp reg) is not rsp reg in IREmitter::visitLEAVE64");
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::RBP && "opr 2(rbp reg) is not rbp reg in IREmitter::visitLEAVE64");
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::RSP && "opr 3(rsp reg) is not rsp reg in IREmitter::visitLEAVE64");

  IRB->SetInsertPoint(BB);

  // mov %rbp, %rsp
  store_reg_val(X86::RSP, get_reg_val(X86::RBP));

  // mov (%rsp), %rbp
  store_reg_val(X86::RBP, get_mem_val(X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, 64));

  // rsp = rsp + 8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateAdd(lhs_val, rhs_val));
}

define_visit(JMP64r)
{
  assert(I->getNumOperands()==1);
  MachineOperand& target_opr = I->getOperand(0);
  assert(target_opr.isReg());

  IRB->SetInsertPoint(BB);

  // jmp target
  IRB->CreateUnreachable();
}

define_visit(JA_1)
{
  assert(I->getNumOperands()==2 && "JA_1's opr's num is not 2");
  MachineOperand& off_opr = I->getOperand(0);
  assert(off_opr.isImm() && "opr 0(off) is not imm in IREmitter::visitJA_1");
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::EFLAGS && "opr 1(efalgs) is not eflags in IREmitter::visitJA_1");

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  int64_t off = off_opr.getImm();

  Value* cf_val = get_CF_val();
  Value* zf_val = get_ZF_val();
  Value* cond_val = IRB->CreateAnd(IRB->CreateICmpEQ(cf_val, ConstantInt::getFalse(*context)), IRB->CreateICmpEQ(zf_val, ConstantInt::getFalse(*context)));

  std::stringstream true_bb_name;
  true_bb_name << "bb_" << (I->getDebugLoc().getLine()+ I->getDesc().getSize() + off);
  BasicBlock* true_bb  = Dec->getOrCreateBasicBlock(true_bb_name.str(), BB->getParent());
  std::stringstream false_bb_name;
  false_bb_name << "bb_" << (I->getDebugLoc().getLine()+ I->getDesc().getSize());
  BasicBlock* false_bb  = Dec->getOrCreateBasicBlock(false_bb_name.str(), BB->getParent());
  IRB->CreateCondBr(cond_val, true_bb, false_bb);
}

define_visit(JE_1)
{
  assert(I->getNumOperands()==2 && "JE_1's opr's num is not 2");
  MachineOperand& off_opr = I->getOperand(0);
  assert(off_opr.isImm() && "opr 0(off) is not imm in IREmitter::visitJE_1");
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::EFLAGS && "opr 1(efalgs) is not eflags in IREmitter::visitJE_1");

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  int64_t off = off_opr.getImm();

  Value* zf_val = get_ZF_val();
  Value* cond_val = IRB->CreateICmpEQ(zf_val, ConstantInt::getTrue(*context));

  std::stringstream true_bb_name;
  true_bb_name << "bb_" << (I->getDebugLoc().getLine()+ I->getDesc().getSize() + off);
  BasicBlock* true_bb  = Dec->getOrCreateBasicBlock(true_bb_name.str(), BB->getParent());
  std::stringstream false_bb_name;
  false_bb_name << "bb_" << (I->getDebugLoc().getLine()+ I->getDesc().getSize());
  BasicBlock* false_bb  = Dec->getOrCreateBasicBlock(false_bb_name.str(), BB->getParent());
  IRB->CreateCondBr(cond_val, true_bb, false_bb);
}

define_visit(JNE_1)
{
  assert(I->getNumOperands()==2 && "JE_1's opr's num is not 2");
  MachineOperand& off_opr = I->getOperand(0);
  assert(off_opr.isImm() && "opr 0(off) is not imm in IREmitter::visitJNE_1");
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::EFLAGS && "opr 1(efalgs) is not eflags in IREmitter::visitJNE_1");

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  int64_t off = off_opr.getImm();

  Value* zf_val = get_ZF_val();
  Value* cond_val = IRB->CreateICmpEQ(zf_val, ConstantInt::getFalse(*context));

  std::stringstream true_bb_name;
  true_bb_name << "bb_" << (I->getDebugLoc().getLine()+ I->getDesc().getSize() + off);
  BasicBlock* true_bb  = Dec->getOrCreateBasicBlock(true_bb_name.str(), BB->getParent());
  std::stringstream false_bb_name;
  false_bb_name << "bb_" << (I->getDebugLoc().getLine()+ I->getDesc().getSize());
  BasicBlock* false_bb  = Dec->getOrCreateBasicBlock(false_bb_name.str(), BB->getParent());
  IRB->CreateCondBr(cond_val, true_bb, false_bb);
}

define_visit(JMP)
{
  assert(I->getNumOperands()==1);
  MachineOperand& off_opr = I->getOperand(0);
  assert(off_opr.isImm());

  IRB->SetInsertPoint(BB);

  int64_t off = off_opr.getImm();

  std::stringstream bb_name;
  bb_name << "bb_" << (I->getDebugLoc().getLine()+ I->getDesc().getSize() + off);
  BasicBlock* bb  = Dec->getOrCreateBasicBlock(bb_name.str(), BB->getParent());
  IRB->CreateBr(bb);
}

define_visit(CALL64pcrel32)
{
  assert(I->getNumOperands()==2);
  MachineOperand& off_opr = I->getOperand(0);
  assert(off_opr.isImm());
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::RSP);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();  

 // rsp = rsp -8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateSub(lhs_val, rhs_val));

  // mov %rip, (%rsp)
  // read %rip val
  Value* src_val = get_reg_val(X86::RIP);

  // compute
  Value* result = src_val;

  store_mem_val(X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, result);

  // jmp target
  int64_t off = off_opr.getImm();
  int64_t target = I->getDebugLoc().getLine()+ I->getDesc().getSize() + off;
  Function* target_func = Dec->getFunctionByAddr(target);
  if(target_func)
    IRB->CreateCall(target_func);
  else
  {
    const object::SectionRef sec = Dec->getDisassembler()->getSectionByAddress(target);
    StringRef sec_name;
    if(error_code ec = sec.getName(sec_name))
      errx(-1, "get sec's name failed: %s", ec.message().c_str());
    if(sec_name.equals(".plt"))
    {
      std::string func_name = get_func_name_in_plt(target);
      target_func = dyn_cast<Function>(Dec->getModule()->getOrInsertFunction(func_name, FunctionType::get(Type::getVoidTy(*context), false)));
      IRB->CreateCall(target_func);
    }
    else
      IRB->CreateUnreachable();    
  }
}

define_visit(CALL64r)
{
  assert(I->getNumOperands()==2 && "CALL64r's opr's num is not 2");
  MachineOperand& target_opr = I->getOperand(0);
  assert(target_opr.isReg() && "opr 0(target) is not Reg in IREmitter::visitCALL64r");
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::RSP && "opr 1(rsp) is not rsp in IREmitter::visitCALL64r");

  IRB->SetInsertPoint(BB);

 // rsp = rsp -8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateSub(lhs_val, rhs_val));

  // mov %rip, (%rsp)
  // read %rip val
  Value* src_val = get_reg_val(X86::RIP);

  // compute
  Value* result = src_val;

  store_mem_val(X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, result);

 // jmp target
  Value* target_val = get_reg_val(target_opr.getReg());
  IRB->CreateCall(BB->getParent()->getParent()->getFunction("saib_collect_indirect"), target_val);
}

define_visit(CALL64m)
{
  assert(I->getNumOperands()==6);
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg());
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm());
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg());
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm());
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg() && seg_opr.getReg()==X86::NoRegister);
  assert(I->getOperand(5).isReg() && I->getOperand(5).getReg()==X86::RSP);

  IRB->SetInsertPoint(BB);

 // rsp = rsp -8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateSub(lhs_val, rhs_val));

  // mov %rip, (%rsp)
  // read %rip val
  Value* src_val = get_reg_val(X86::RIP);

  // compute
  Value* result = src_val;

  store_mem_val(X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, result);

 // jmp target
//  Value* target_val = get_reg_val(target_opr.getReg());

  IRB->CreateUnreachable();
}

define_visit(RET)
{
  assert(I->getNumOperands()==0 && "RETQ's opr's num is not 0");

  IRB->SetInsertPoint(BB);

  // pop rip
  // mov (%rsp), rip
  // read src
  Value* src_val = get_mem_val(X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, 64);

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(X86::RIP, result);

  // rsp = rsp + 8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateAdd(lhs_val, rhs_val));

  // jmp target
  IRB->CreateRetVoid();
}

define_visit(NOOP)
{
  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  IRB->CreateCall(Intrinsic::getDeclaration(Dec->getModule(), Intrinsic::donothing, {Type::getVoidTy(*context)}));
}

} // End namespace fracture
