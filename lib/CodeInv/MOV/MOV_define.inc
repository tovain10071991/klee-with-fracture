#define define_mov(name) \
  void IREmitter::Mov##name(BasicBlock *BB, MachineInstr* I, unsigned init_size, unsigned final_size)

define_mov(r_)
{
  assert(I->getNumOperands()==2 && "MOV's opr's num is not 2");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && "opr 0(dest) is not reg in IREmitter::visitMOVr");
  MachineOperand& src_opr = I->getOperand(1);

  IRB->SetInsertPoint(BB);

  // read src val
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), init_size, final_size);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_mov(rm)
{
  assert(I->getNumOperands()==6 && "MOV64mr's opr's num is not 6");
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg() && "opr 0(des) is not reg in IREmitter::visitMOV64rm");
  MachineOperand& base_opr = I->getOperand(1);
  assert(base_opr.isReg() && "opr 1(base) is not reg in IREmitter::visitMOV64rm");
  MachineOperand& scale_opr = I->getOperand(2);
  assert(scale_opr.isImm() && "opr 2(scale) is not imm in IREmitter::visitMOV64rm");
  MachineOperand& idx_opr = I->getOperand(3);
  assert(idx_opr.isReg() && "opr 3(idx) is not reg in IREmitter::visitMOV64rm");
  MachineOperand& off_opr = I->getOperand(4);
  assert(off_opr.isImm() && "opr 4(off) is not imm in IREmitter::visitMOV64rm");
  MachineOperand& seg_opr = I->getOperand(5);
  assert(seg_opr.isReg());

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), final_size);

  // compute
  Value* result = src_val;

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_mov(mr)
{
  assert(I->getNumOperands()==6 && "MOVmr's opr's num is not 6");
  MachineOperand& src_opr = I->getOperand(5);
  assert(src_opr.isReg() && "opr 0(des) is not reg in IREmitter::visitMOVmr");
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg() && "opr 1(base) is not reg in IREmitter::visitMOVmr");
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm() && "opr 2(scale) is not imm in IREmitter::visitMOVmr");
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg() && "opr 3(idx) is not reg in IREmitter::visitMOV64mr");
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm() && "opr 4(off) is not imm in IREmitter::visitMOV64mr");
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg());

  IRB->SetInsertPoint(BB);

  // read srcg
  Value* src_val = get_reg_val(src_opr.getReg());

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_mov(sxrr)
{
	assert(I->getNumOperands()==2);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg());
  MachineOperand& src_opr = I->getOperand(1);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();

  // read src val
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), init_size, final_size);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = IRB->CreateSExt(src_val, Type::getIntNTy(*context, Dec->getDisassembler()->getMCDirector()->getRegType(des_opr.getReg()).getSizeInBits()));

  // mask and store result
  store_reg_val(des_opr.getReg(), result);
}

define_visit(MOV32r)
{
	Movr_(BB, I, 32, 32);
}

define_visit(MOV32rm)
{
	Movrm(BB, I, 0, 32);
}

define_visit(MOV64r)
{
	Movr_(BB, I, 64, 64);
}

define_visit(MOV64ri32)
{
	Movr_(BB, I, 32, 64);
}

define_visit(MOV8m)
{
  assert(I->getNumOperands()==6);
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg());
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm());
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg());
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm());
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg());
  MachineOperand& src_opr = I->getOperand(5);

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 8, 8);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_visit(MOV8ri)
{
  Movr_(BB, I, 8, 8);
}

define_visit(MOV32m)
{
  assert(I->getNumOperands()==6 && "MOV32mr's opr's num is not 6");
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg() && "opr 0(base) is not reg in IREmitter::visitMOV32mr");
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm() && "opr 1(scale) is not imm in IREmitter::visitMOV32mr");
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg() && "opr 2(idx) is not reg in IREmitter::visitMOV32mr");
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm() && "opr 3(off) is not imm in IREmitter::visitMOV32mr");
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg());
  MachineOperand& src_opr = I->getOperand(5);

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 32, 32);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_visit(MOV64m)
{
  assert(I->getNumOperands()==6 && "MOV64mr's opr's num is not 6");
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg() && "opr 0(base) is not reg in IREmitter::visitMOV64m");
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm() && "opr 1(scale) is not imm in IREmitter::visitMOV64m");
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg() && "opr 2(idx) is not reg in IREmitter::visitMOV64m");
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm() && "opr 3(off) is not imm in IREmitter::visitMOV64m");
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg());
  MachineOperand& src_opr = I->getOperand(5);

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val;
  if(src_opr.isImm())
  {
    src_val = get_imm_val(src_opr.getImm(), 64, 64);
  }
  else
  {
    src_val = get_reg_val(src_opr.getReg());
  }

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_visit(MOV64mi32)
{
  assert(I->getNumOperands()==6 && "MOV64mr's opr's num is not 6");
  MachineOperand& base_opr = I->getOperand(0);
  assert(base_opr.isReg() && "opr 0(base) is not reg in IREmitter::visitMOV64m");
  MachineOperand& scale_opr = I->getOperand(1);
  assert(scale_opr.isImm() && "opr 1(scale) is not imm in IREmitter::visitMOV64m");
  MachineOperand& idx_opr = I->getOperand(2);
  assert(idx_opr.isReg() && "opr 2(idx) is not reg in IREmitter::visitMOV64m");
  MachineOperand& off_opr = I->getOperand(3);
  assert(off_opr.isImm() && "opr 3(off) is not imm in IREmitter::visitMOV64m");
  MachineOperand& seg_opr = I->getOperand(4);
  assert(seg_opr.isReg());
  MachineOperand& src_opr = I->getOperand(5);
  assert(src_opr.isImm() && "opr 5(imm) is not imm in IREmitter::visitMOV64m");

  IRB->SetInsertPoint(BB);

  // read src
  Value* src_val = get_imm_val(src_opr.getImm(), 32, 64);

  // compute
  Value* result = src_val;

  // mask and store result
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
}

define_visit(MOV64rm)
{
  Movrm(BB, I, 0, 64);
}

// define_visit(MOVZX16rr8)
// {
// }

// define_visit(MOVZX32rr8)
// {
// }

// define_visit(MOVZX32rr16)
// {
// }

// define_visit(MOVZX16rm8)
// {
// }

// define_visit(MOVZX32rm8)
// {
// }

// define_visit(MOVZX32rm16)
// {
// }

// define_visit(MOVSX16rr8)
// {
// }

// define_visit(MOVSX32rr16)
// {
// }

// define_visit(MOVSX32rr8)
// {
// }

// define_visit(MOVSX64rr8)
// {
// }

// define_visit(MOVSX64rr16)
// {
// }

define_visit(MOVSX64rr32)
{
	Movsxrr(BB, I, 0, 0);
}

// define_visit(MOVSX16rm8)
// {
// }

// define_visit(MOVSX32rm8)
// {
// }

// define_visit(MOVSX32rm16)
// {
// }

// define_visit(MOVSX64rm8)
// {
// }

// define_visit(MOVSX64rm16)
// {
// }

// define_visit(MOVSX64rm32)
// {
// }

define_visit(MOVUPSrm)
{
	Movrm(BB, I, 0, 128);
}

define_visit(MOVUPSmr)
{
	Movmr(BB, I, 0, 0);
}

// define_visit(MOVUPSrr)
// {
// }

// define_visit(MOVUPSrr_REV)
// {
// }