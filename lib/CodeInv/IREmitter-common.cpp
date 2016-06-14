#include "CodeInv/IREmitter.h"
#include "CodeInv/Decompiler.h"
#define GET_REGINFO_ENUM
#include "../lib/Target/X86/X86GenRegisterInfo.inc"

using namespace llvm;

namespace fracture {

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

Value* IREmitter::get_pointer_val(BasicBlock* BB, unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg)
{
  LLVMContext* context = Dec->getContext();

  Value* seg_val = seg_reg==X86::NoRegister ? ConstantInt::get(Type::getInt64Ty(*context), 0) : get_reg_val(seg_reg);
  Value* base_val = base_reg==X86::NoRegister ? ConstantInt::get(Type::getInt64Ty(*context), 0) : get_reg_val(base_reg);
  Value* scale_val = ConstantInt::get(Type::getInt64Ty(*context), scale);
  Value* idx_val = idx_reg==X86::NoRegister ? ConstantInt::get(Type::getInt64Ty(*context), 0) : get_reg_val(idx_reg);
  Value* offset_val = ConstantInt::get(Type::getInt64Ty(*context), offset);
  assert(seg_reg != X86::CS && seg_reg != X86::SS);
  if(seg_reg == X86::FS)
    seg_val = IRB->CreateLoad(Dec->getModule()->getGlobalVariable("FS_BASE"));
  else if(seg_reg == X86::GS)
    seg_val = IRB->CreateLoad(Dec->getModule()->getGlobalVariable("GS_BASE"));
  return IRB->CreateAdd(seg_val, IRB->CreateAdd(base_val, IRB->CreateAdd(offset_val, IRB->CreateMul(idx_val, scale_val))));
}

Value* IREmitter::get_mem_val(BasicBlock* BB, unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg, unsigned mem_size)
{
  LLVMContext* context = Dec->getContext();

  Value* pointer = IRB->CreateIntToPtr(get_pointer_val(BB, base_reg, scale, idx_reg, offset, seg_reg), PointerType::get(Type::getIntNTy(*context, mem_size), 0));

  return IRB->CreateLoad(pointer);
}

void IREmitter::store_mem_val(BasicBlock* BB, unsigned base_reg, int64_t scale, unsigned idx_reg, int64_t offset, unsigned seg_reg, Value* val)
{
  Value* pointer = IRB->CreateIntToPtr(get_pointer_val(BB, base_reg, scale, idx_reg, offset, seg_reg), PointerType::get(val->getType(), 0));

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

} // end of namespace fracture