void IREmitter::Incr (BasicBlock *BB, MachineInstr* I)
{
  assert(I->getNumOperands() == 3);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg() == lhs_opr.getReg());
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg() == X86::EFLAGS);
  
  IRB->SetInsertPoint(BB);
  
  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());
  
  LLVMContext* context = Dec->getContext();
  
  // compute
  Value* result = IRB->CreateAdd(lhs_val, ConstantInt::getTrue(*context));
  
  // writeback
  store_reg_val(des_opr.getReg(), result);
  
  store_AF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
  store_PF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
  store_ZF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
  store_SF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
  store_OF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
}

// define_visit(INC16m)
// {
// }

// define_visit(INC32m)
// {
// }

// define_visit(INC8m)
// {
// }

// define_visit(INC8r)
// {
// }

// define_visit(INC16r)
// {
// }

// define_visit(INC32r)
// {
// }

// define_visit(INC64r)
// {
// }

define_visit(INC64_32r)
{
	Incr(BB, I);
}

// define_visit(INC64_16r)
// {
// }

// define_visit(INC64m)
// {
// }

// define_visit(INC64_32m)
// {
// }

// define_visit(INC64_16m)
// {
// }