define_visit(JMP64r)
{
  assert(I->getNumOperands()==1);
  MachineOperand& target_opr = I->getOperand(0);
  assert(target_opr.isReg());

  IRB->SetInsertPoint(BB);

  // jmp target
  IRB->CreateUnreachable();
}

define_visit(JMP)
{
  assert(I->getNumOperands()==1);
  MachineOperand& off_opr = I->getOperand(0);
  assert(off_opr.isImm());

  IRB->SetInsertPoint(BB);

  int64_t off = off_opr.getImm();

  std::stringstream bb_name;
  bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc())+ I->getDesc().getSize() + off);
  BasicBlock* bb  = Dec->getOrCreateBasicBlock(bb_name.str(), BB->getParent());
  IRB->CreateBr(bb);
}