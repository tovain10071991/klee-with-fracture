define_visit(XORPSrr)
{
  assert(I->getNumOperands()==3);
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(lhs_opr.isReg());
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg());

  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val = get_reg_val(rhs_opr.getReg());

  // compute
  Value* result = IRB->CreateXor(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);
}