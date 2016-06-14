int64_t init_Jcc(MachineInstr* I)
{
  assert(I->getNumOperands()==2 && "opr's num is not 2");
  MachineOperand& off_opr = I->getOperand(0);
  assert(off_opr.isImm() && "opr 0(off) is not imm");
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::EFLAGS && "opr 1(efalgs) is not eflags");
  return off_opr.getImm();
}

void IREmitter::Jcc(BasicBlock *BB, MachineInstr* I, Value* cond_val, int64_t off)
{
  std::stringstream true_bb_name;
  true_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc())+ I->getDesc().getSize() + off);
  BasicBlock* true_bb  = Dec->getOrCreateBasicBlock(true_bb_name.str(), BB->getParent());
  std::stringstream false_bb_name;
  false_bb_name << "bb_" << (Dec->getDisassembler()->getDebugOffset(I->getDebugLoc())+ I->getDesc().getSize());
  BasicBlock* false_bb  = Dec->getOrCreateBasicBlock(false_bb_name.str(), BB->getParent());
  IRB->CreateCondBr(cond_val, true_bb, false_bb);
}

// CF=0 and ZF=0
define_visit(JA_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* cf_val = get_CF_val();
  Value* zf_val = get_ZF_val();
  Value* cond_val = IRB->CreateAnd(IRB->CreateICmpEQ(cf_val, ConstantInt::getFalse(*context)), IRB->CreateICmpEQ(zf_val, ConstantInt::getFalse(*context)));

  Jcc(BB, I, cond_val, off);
}

// CF=0
define_visit(JAE_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* cf_val = get_CF_val();
  Value* cond_val = IRB->CreateICmpEQ(cf_val, ConstantInt::getFalse(*context));

  Jcc(BB, I, cond_val, off);
}

// CF=1 or ZF=1
define_visit(JBE_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();
  
  Value* cf_val = get_CF_val();
  Value* zf_val = get_ZF_val();
  Value* cond_val = IRB->CreateOr(IRB->CreateICmpEQ(cf_val, ConstantInt::getTrue(*context)), IRB->CreateICmpEQ(zf_val, ConstantInt::getTrue(*context)));

  Jcc(BB, I, cond_val, off);
}
// CF=1
define_visit(JB_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();
  
  Value* cf_val = get_CF_val();
  Value* cond_val = IRB->CreateICmpEQ(cf_val, ConstantInt::getTrue(*context));

  Jcc(BB, I, cond_val, off);
}
//  ZF=1
define_visit(JE_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();
  
  Value* zf_val = get_ZF_val();
  Value* cond_val = IRB->CreateICmpEQ(zf_val, ConstantInt::getTrue(*context));

  Jcc(BB, I, cond_val, off);
}
// ZF=0 and SF=OF
define_visit(JG_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* zf_val = get_ZF_val();
  Value* sf_val = get_SF_val();
  Value* of_val = get_OF_val();
  Value* cond_val = IRB->CreateAnd(IRB->CreateICmpEQ(zf_val, ConstantInt::getFalse(*context)), IRB->CreateICmpEQ(sf_val, of_val));

  Jcc(BB, I, cond_val, off);
}
// SF=OF
define_visit(JGE_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);

  Value* sf_val = get_SF_val();
  Value* of_val = get_OF_val();
  Value* cond_val = IRB->CreateICmpEQ(sf_val, of_val);

  Jcc(BB, I, cond_val, off);
}
// SF≠ OF
define_visit(JL_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);

  Value* sf_val = get_SF_val();
  Value* of_val = get_OF_val();
  Value* cond_val = IRB->CreateICmpNE(sf_val, of_val);

  Jcc(BB, I, cond_val, off);
}
// ZF=1 or SF≠ OF
define_visit(JLE_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* zf_val = get_ZF_val();
  Value* sf_val = get_SF_val();
  Value* of_val = get_OF_val();
  Value* cond_val = IRB->CreateOr(IRB->CreateICmpEQ(zf_val, ConstantInt::getTrue(*context)), IRB->CreateICmpNE(sf_val, of_val));

  Jcc(BB, I, cond_val, off);
}
// ZF=0
define_visit(JNE_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* zf_val = get_ZF_val();
  Value* cond_val = IRB->CreateICmpEQ(zf_val, ConstantInt::getFalse(*context));

  Jcc(BB, I, cond_val, off);
}
// OF=0
define_visit(JNO_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* of_val = get_OF_val();
  Value* cond_val = IRB->CreateICmpEQ(of_val, ConstantInt::getFalse(*context));

  Jcc(BB, I, cond_val, off);
}
// PF=0
define_visit(JNP_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* pf_val = get_PF_val();
  Value* cond_val = IRB->CreateICmpEQ(pf_val, ConstantInt::getFalse(*context));

  Jcc(BB, I, cond_val, off);
}
// SF=0
define_visit(JNS_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* sf_val = get_SF_val();
  Value* cond_val = IRB->CreateICmpEQ(sf_val, ConstantInt::getFalse(*context));

  Jcc(BB, I, cond_val, off);
}
// OF=1
define_visit(JO_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* of_val = get_OF_val();
  Value* cond_val = IRB->CreateICmpEQ(of_val, ConstantInt::getTrue(*context));

  Jcc(BB, I, cond_val, off);
}
// PF=1
define_visit(JP_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* pf_val = get_PF_val();
  Value* cond_val = IRB->CreateICmpEQ(pf_val, ConstantInt::getTrue(*context));

  Jcc(BB, I, cond_val, off);
}
// SF=1
define_visit(JS_1)
{
  int64_t off = init_Jcc(I);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  Value* sf_val = get_SF_val();
  Value* cond_val = IRB->CreateICmpEQ(sf_val, ConstantInt::getTrue(*context));

  Jcc(BB, I, cond_val, off);
}