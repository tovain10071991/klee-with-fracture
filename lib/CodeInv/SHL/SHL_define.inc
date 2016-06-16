void IREmitter::Shlri(BasicBlock *BB, MachineInstr* I, unsigned init_size, unsigned final_size)
{
  assert(I->getNumOperands() == 4);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg());
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg() && des_opr.getReg() == lhs_opr.getReg());
  MachineOperand& rhs_opr = I->getOperand(2);
  assert(rhs_opr.isImm());
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg() == X86::EFLAGS);
  
  IRB->SetInsertPoint(BB);
  
  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());
  
  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), init_size, final_size);
  
  // compute
  Value* result = IRB->CreateShl(lhs_val, rhs_val);
  
  // writeback
  store_reg_val(des_opr.getReg(), result);
  
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

// define_visit(SHL16m1)
// {
// }
// define_visit(SHL16mCL)
// {
// }
// define_visit(SHL16mi)
// {
// }
// define_visit(SHL16r1)
// {
// }
// define_visit(SHL16rCL)
// {
// }
// define_visit(SHL16ri)
// {
// }
// define_visit(SHL32m1)
// {
// }
// define_visit(SHL32mCL)
// {
// }
// define_visit(SHL32mi)
// {
// }
// define_visit(SHL32r1)
// {
// }
// define_visit(SHL32rCL)
// {
// }

define_visit(SHL32ri)
{
  Shlri(BB, I, 8, 32);
}

define_visit(SHL64ri)
{
  Shlri(BB, I, 8, 64);
}

// define_visit(SHL8m1)
// {
// }

// define_visit(SHL8mCL)
// {
// }

// define_visit(SHL8mi)
// {
// }

// define_visit(SHL8r1)
// {
// }

// define_visit(SHL8rCL)
// {
// }

define_visit(SHL8ri)
{
  Shlri(BB, I, 8, 8);
}
