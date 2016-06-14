define_visit(NEG32r)
{
  assert(I->getNumOperands()==3);
  MachineOperand& src_opr = I->getOperand(1);
  assert(src_opr.isReg());
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==src_opr.getReg());
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS);
  
  IRB->SetInsertPoint(BB);

  //read src
  Value* src_val = get_reg_val(src_opr.getReg());

  // compute
  Value* result = IRB->CreateNeg(src_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  // set CF
  IRB->CreateStore(IRB->CreateICmpNE(src_val, ConstantInt::get(src_val->getType(), 0)), Dec->getModule()->getGlobalVariable("CF"));
}