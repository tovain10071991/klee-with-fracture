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

  store_mem_val(BB, X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, result);
}