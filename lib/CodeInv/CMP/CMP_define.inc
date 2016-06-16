#define define_cmp(name) \
  void IREmitter::Cmp##name(BasicBlock *BB, MachineInstr* I, unsigned init_size, unsigned final_size)

define_cmp(r_)
{
  assert(I->getNumOperands()==3);
  MachineOperand& lhs_opr = I->getOperand(0);
  assert(lhs_opr.isReg());
  MachineOperand& rhs_opr = I->getOperand(1);
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS);
  
  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val;
  if(rhs_opr.isImm())
  {
    rhs_val = get_imm_val(rhs_opr.getImm(), init_size, final_size);
  }
  else
  {
    rhs_val = get_reg_val(rhs_opr.getReg());
  }

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_cmp(rm)
{
  assert(I->getNumOperands()==7);
  MachineOperand& lhs_opr = I->getOperand(0);
  assert(lhs_opr.isReg());
  MachineOperand& base_opr = I->getOperand(1);
  assert(base_opr.isReg());
  MachineOperand& scale_opr = I->getOperand(2);
  assert(scale_opr.isImm());
  MachineOperand& idx_opr = I->getOperand(3);
  assert(idx_opr.isReg());
  MachineOperand& off_opr = I->getOperand(4);
  assert(off_opr.isImm());
  MachineOperand& seg_opr = I->getOperand(5);
  assert(seg_opr.isReg());
  assert(I->getOperand(6).isReg() && I->getOperand(6).getReg()==X86::EFLAGS);

  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Value* rhs_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), final_size);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_cmp(mi)
{
  assert(I->getNumOperands()==7);
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
  MachineOperand& rhs_opr = I->getOperand(5);
  assert(rhs_opr.isImm());
  assert(I->getOperand(6).isReg() && I->getOperand(6).getReg()==X86::EFLAGS);

  IRB->SetInsertPoint(BB);

  // read lhs
  Value* lhs_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), final_size);

  //read rhs
  Value* rhs_val = get_imm_val(rhs_opr.getImm(), init_size, final_size);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(CMP8rr)
{
	Cmpr_(BB, I, 0, 0);
}

// define_visit(CMP8rr_REV)
// {
// }

define_visit(CMP16rr)
{
	Cmpr_(BB, I, 0, 0);
}

// define_visit(CMP16rr_REV)
// {
// }

// define_visit(CMP32rr_REV)
// {
// }

define_visit(CMP32rr)
{
	Cmpr_(BB, I, 0, 0);
}

define_visit(CMP64rr)
{
	Cmpr_(BB, I, 0, 0);
}

define_visit(CMP8ri)
{
	Cmpr_(BB, I, 8, 8);
}

// define_visit(CMP8i8)
// {
// }

define_visit(CMP16ri)
{
	Cmpr_(BB, I, 16, 16);
}

define_visit(CMP16ri8)
{
	Cmpr_(BB, I, 8, 16);
}

// define_visit(CMP16i16)
// {
// }

// define_visit(CMP32i32)
// {
// }

define_visit(CMP32ri)
{
	Cmpr_(BB, I, 32, 32);
}

define_visit(CMP32ri8)
{
	Cmpr_(BB, I, 8, 32);
}

define_visit(CMP64ri32)
{
	Cmpr_(BB, I, 32, 64);
}

define_visit(CMP64ri8)
{
	Cmpr_(BB, I, 8, 64);
}

// define_visit(CMP64i32)
// {
// }

define_visit(CMP32mi8)
{
	Cmpmi(BB, I, 8, 32);
}

define_visit(CMP8mi)
{
	Cmpmi(BB, I, 8, 8);
}

define_visit(CMP16mi)
{
	Cmpmi(BB, I, 16, 16);
}

define_visit(CMP32mi)
{
	Cmpmi(BB, I, 32, 32);
}

define_visit(CMP8rm)
{
	Cmprm(BB, I, 0, 8);
}

define_visit(CMP16rm)
{
	Cmprm(BB, I, 0, 16);
}

define_visit(CMP32rm)
{
	Cmprm(BB, I, 0, 32);
}

// define_visit(CMP8mr)
// {
// }

// define_visit(CMP16mr)
// {
// }

// define_visit(CMP32mr)
// {
// }

define_visit(CMP16mi8)
{
	Cmpmi(BB, I, 8, 16);
}

define_visit(CMP64mi8)
{
	Cmpmi(BB, I, 8, 64);
}

define_visit(CMP64mi32)
{
	Cmpmi(BB, I, 32, 64);
}

define_visit(CMP64rm)
{
	Cmprm(BB, I, 0, 64);
}

// define_visit(CMP64mr)
// {
// }