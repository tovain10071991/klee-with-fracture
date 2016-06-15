define_visit(LEA64r)
{
  assert(I->getNumOperands()==6 && "LEA64r's opr's num is not 6");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && "opr 0(des) is not reg in IREmitter::visitLEA64r");
  MachineOperand& base_opr = I->getOperand(1);
  assert(base_opr.isReg() && "opr 1(base) is not reg in IREmitter::visitLEA64r");
  MachineOperand& scale_opr = I->getOperand(2);
  assert(scale_opr.isImm() && "opr 2(scale) is not imm in IREmitter::visitLEA64r");
  MachineOperand& idx_opr = I->getOperand(3);
  assert(idx_opr.isReg() && "opr 3(idx) is not reg in IREmitter::visitLEA64r");
  MachineOperand& off_opr = I->getOperand(4);
  assert(off_opr.isImm() && "opr 4(off) is not imm in IREmitter::visitLEA64r");
  MachineOperand& seg_opr = I->getOperand(5);
  assert(seg_opr.isReg());

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val = get_pointer_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg());

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}