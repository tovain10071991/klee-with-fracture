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