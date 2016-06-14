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