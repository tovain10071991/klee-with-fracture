void IREmitter::Decr (BasicBlock *BB, MachineInstr* I)
{
  assert(I->getNumOperands() == 3);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg() == lhs_opr.getReg());
  assert(
	  I->getOperand(2).isReg() && I->getOperand(2).getReg() == X86::EFLAGS);

  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  LLVMContext* context = Dec->getContext();

  // compute
  Value* result = IRB->CreateSub(lhs_val, ConstantInt::getTrue(*context));

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
  store_PF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
  store_ZF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
  store_SF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
  store_OF_val(I->getOpcode(), lhs_val, ConstantInt::getTrue(*context), result);
}

define_visit(DEC16r)
{
  Decr(BB, I);
}

define_visit(DEC8r)
{
  Decr(BB, I);
}

// define_visit(DEC16m)
// {
// }

// define_visit(DEC32m)
// {
// }

// define_visit(DEC8m)
// {
// }

define_visit(DEC32r)
{
  Decr(BB, I);
}

// define_visit(DEC64_16r)
// {
// }

// define_visit(DEC64_32r)
// {
// }

define_visit(DEC64r)
{
  Decr(BB, I);
}