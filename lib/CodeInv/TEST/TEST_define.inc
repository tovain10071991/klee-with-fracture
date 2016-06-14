void IREmitter::Testrr(BasicBlock *BB, MachineInstr* I)
{
  assert(I->getNumOperands()==3 && "TESTrr's opr's num is not 3");
  MachineOperand& lhs_opr = I->getOperand(0);
  assert(lhs_opr.isReg() && "opr 0(lhs reg) is not reg in IREmitter::visitTESTrr");
  MachineOperand& rhs_opr = I->getOperand(1);
  assert(rhs_opr.isReg() && "opr 1(rhs reg) is not reg in IREmitter::visitTESTrr");
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS && "opr 2(efalgs) is not eflags in IREmitter::visitTESTrr");
  
  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();
  
  // read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());
  
  //read rhs
  Value* rhs_val = get_reg_val(rhs_opr.getReg());
  
  // compute
  Value* result = IRB->CreateAnd(lhs_val, rhs_val);
  
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("CF"));
  IRB->CreateStore(ConstantInt::getFalse(*context), Dec->getModule()->getGlobalVariable("OF"));
}

define_visit(TEST32rr)
{
  Testrr(BB, I);
}

define_visit(TEST64rr)
{
  Testrr(BB, I);
}