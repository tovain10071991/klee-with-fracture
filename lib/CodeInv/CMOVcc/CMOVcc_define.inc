define_visit(CMOVS64rr)
{
  assert(I->getNumOperands()==4);
  MachineOperand& src_opr = I->getOperand(2);
  assert(src_opr.isReg());
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg());
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==des_opr.getReg());  
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg()==X86::EFLAGS);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();
  
  Value* sf_val = get_SF_val();

  Value* cond_val = IRB->CreateICmpEQ(sf_val, ConstantInt::getTrue(*context));
  
  std::stringstream true_bb_name;
  true_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc())) << "_mov";
  BasicBlock* true_bb  = Dec->getOrCreateBasicBlock(true_bb_name.str(), BB->getParent());
  std::stringstream false_bb_name;
  false_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc()) + I->getDesc().getSize());
  BasicBlock* false_bb  = Dec->getOrCreateBasicBlock(false_bb_name.str(), BB->getParent());
  IRB->CreateCondBr(cond_val, true_bb, false_bb);
  
  IRB->SetInsertPoint(true_bb);
  Value* src_val = get_reg_val(src_opr.getReg());
  Value* result = src_val;
  store_reg_val(des_opr.getReg(), result);
  IRB->CreateBr(false_bb);
}