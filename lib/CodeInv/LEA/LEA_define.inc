void IREmitter::Lea(BasicBlock *BB, MachineInstr* I)
{
  assert(I->getNumOperands()==6 && "LEA's opr's num is not 6");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && "opr 0(des) is not reg in IREmitter::visitLEA");
  MachineOperand& base_opr = I->getOperand(1);
  assert(base_opr.isReg() && "opr 1(base) is not reg in IREmitter::visitLEA");
  MachineOperand& scale_opr = I->getOperand(2);
  assert(scale_opr.isImm() && "opr 2(scale) is not imm in IREmitter::visitLEA");
  MachineOperand& idx_opr = I->getOperand(3);
  assert(idx_opr.isReg() && "opr 3(idx) is not reg in IREmitter::visitLEA64r");
  MachineOperand& off_opr = I->getOperand(4);
  assert(off_opr.isImm() && "opr 4(off) is not imm in IREmitter::visitLEA64r");
  MachineOperand& seg_opr = I->getOperand(5);
  assert(seg_opr.isReg() && seg_opr.getReg()==X86::NoRegister && "opr 5(seg) is not noreg in IREmitter::visitLEA");

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val = get_pointer_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg());

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_visit(LEA16r)
{
  Lea(BB, I);
}

define_visit(LEA32r)
{
  Lea(BB, I);
}

define_visit(LEA64r)
{
  Lea(BB, I);
}

define_visit(LEA64_32r)
{
  Lea(BB, I);
}