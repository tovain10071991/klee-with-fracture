define_visit(LEAVE64)
{
  assert(I->getNumOperands()==4 && "LEAVE64's opr's num is not 4");
  assert(I->getOperand(0).isReg() && I->getOperand(0).getReg()==X86::RBP && "opr 0(rbp reg) is not rbp reg in IREmitter::visitLEAVE64");
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::RSP && "opr 1(rsp reg) is not rsp reg in IREmitter::visitLEAVE64");
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::RBP && "opr 2(rbp reg) is not rbp reg in IREmitter::visitLEAVE64");
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::RSP && "opr 3(rsp reg) is not rsp reg in IREmitter::visitLEAVE64");

  IRB->SetInsertPoint(BB);

  // mov %rbp, %rsp
  store_reg_val(X86::RSP, get_reg_val(X86::RBP));

  // mov (%rsp), %rbp
  store_reg_val(X86::RBP, get_mem_val(BB, X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, 64));

  // rsp = rsp + 8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateAdd(lhs_val, rhs_val));
}