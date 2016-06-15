define_visit(STOSQ)
{
  assert(I->getNumOperands()==6);
  assert(I->getOperand(0).isReg() && (I->getOperand(0).getReg() == X86::RCX || I->getOperand(0).getReg() == X86::ECX || I->getOperand(0).getReg() == X86::CX));
  assert(I->getOperand(3).isReg() && I->getOperand(3).getReg() == I->getOperand(0).getReg());  
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg() == X86::RAX);
  
  MachineOperand& base_opr = I->getOperand(4);
  assert(base_opr.isReg() && base_opr.getReg() == X86::RDI);
  MachineOperand& des_opr = I->getOperand(1);
  assert(des_opr.isReg() && des_opr.getReg() == base_opr.getReg());
  assert(I->getOperand(5).isReg() && I->getOperand(5).getReg()==X86::EFLAGS);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();
  
  Value* src_val = get_reg_val(X86::RAX);
  store_mem_val(base_opr.getReg(), 0, X86::NoRegister, 0, X86::NoRegister, src_val);

  Value* des_val = get_reg_val(des_opr.getReg());
  Value* df_val = get_DF_val();
  Value* result = IRB->CreateAdd(des_val, IRB->CreateAdd(IRB->CreateMul(IRB->CreateSExt(df_val, Type::getIntNTy(*context, Dec->getDisassembler()->getMCDirector()->getRegType(des_opr.getReg()).getSizeInBits())), ConstantInt::get(Type::getIntNTy(*context, Dec->getDisassembler()->getMCDirector()->getRegType(des_opr.getReg()).getSizeInBits()), -2)), ConstantInt::get(Type::getIntNTy(*context, Dec->getDisassembler()->getMCDirector()->getRegType(des_opr.getReg()).getSizeInBits()), 1)));
  
  store_reg_val(des_opr.getReg(), result);
  
  if(has_prefix)
  {
    has_prefix = false;
    std::stringstream circuit_bb_name;
    circuit_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc())) << "_circuit";
    BasicBlock* circuit_bb  = Dec->getOrCreateBasicBlock(circuit_bb_name.str(), BB->getParent());
    IRB->CreateBr(circuit_bb);
  }
  // else
  // {
    // std::stringstream out_bb_name;
    // // out_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc()) + I->getDesc().getSize());
    // BasicBlock* out_bb  = Dec->getOrCreateBasicBlock(out_bb_name.str(), BB->getParent());
    // IRB->CreateBr(out_bb);
  // }
}