define_visit(RET)
{
  assert(I->getNumOperands()==0 && "RETQ's opr's num is not 0");

  IRB->SetInsertPoint(BB);

  // pop rip
  // mov (%rsp), rip
  // read src
  Value* src_val = get_mem_val(X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, 64);

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(X86::RIP, result);

  // rsp = rsp + 8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateAdd(lhs_val, rhs_val));

  // jmp target
  IRB->CreateRetVoid();
}