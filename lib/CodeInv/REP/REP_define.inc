define_visit(REP)
{
  assert(I->getNumOperands()==3);
  MachineOperand& src_opr = I->getOperand(0);
  assert(src_opr.isReg() && (src_opr.getReg() == X86::RCX || src_opr.getReg() == X86::ECX || src_opr.getReg() == X86::CX));
  MachineOperand& des_opr = I->getOperand(1);
  assert(des_opr.isReg() && des_opr.getReg() == src_opr.getReg());
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();
  has_prefix = true;

  // if(rcx!=0)
  
  Value* src_val = get_reg_val(src_opr.getReg());
  
  Value* cond_val = IRB->CreateICmpNE(src_val, ConstantInt::get(Type::getIntNTy(*context, Dec->getDisassembler()->getMCDirector()->getRegType(src_opr.getReg()).getSizeInBits()), 0));
  
  std::stringstream true_bb_name;
  true_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc()) + I->getDesc().getSize());
  BasicBlock* true_bb  = Dec->getOrCreateBasicBlock(true_bb_name.str(), BB->getParent());
  std::stringstream false_bb_name;
  false_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc()) + I->getDesc().getSize() + I->getNextNode()->getDesc().getSize());
  BasicBlock* false_bb  = Dec->getOrCreateBasicBlock(false_bb_name.str(), BB->getParent());
  IRB->CreateCondBr(cond_val, true_bb, false_bb);
  
  std::stringstream circuit_bb_name;
  circuit_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc()) + I->getDesc().getSize()) << "_circuit";
  BasicBlock* circuit_bb  = Dec->getOrCreateBasicBlock(circuit_bb_name.str(), BB->getParent());
  
  IRB->SetInsertPoint(circuit_bb);
  IRB->CreateSub(src_val, ConstantInt::get(Type::getIntNTy(*context, Dec->getDisassembler()->getMCDirector()->getRegType(src_opr.getReg()).getSizeInBits()), 1));
  IRB->CreateBr(BB);
}