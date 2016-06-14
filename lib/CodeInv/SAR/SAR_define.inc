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