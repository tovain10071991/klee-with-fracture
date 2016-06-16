#define define_add(name) \
  void IREmitter::Add##name(BasicBlock *BB, MachineInstr* I, unsigned init_size, unsigned final_size)

define_add(ri)
{
	assert(I->getNumOperands() == 4);
	MachineOperand& lhs_opr = I->getOperand(1);
	assert(lhs_opr.isReg());
	MachineOperand& rhs_opr = I->getOperand(2);
	assert(I->getOperand(3).isReg() && I->getOperand(3).getReg() == X86::EFLAGS);
	MachineOperand& des_opr = I->getOperand(0);
	assert(des_opr.isReg() && des_opr.getReg() == lhs_opr.getReg());

	IRB->SetInsertPoint(BB);

	//read lhs
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
	Value* result = IRB->CreateAdd(lhs_val, rhs_val);

	// writeback
	store_reg_val(des_opr.getReg(), result);

	store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
	store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
	store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
	store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
	store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
	store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_add(rm)
{
  assert(I->getNumOperands()==8);
  MachineOperand& des_opr = I->getOperand(0);
  assert(des_opr.isReg());
  MachineOperand& lhs_opr = I->getOperand(1);
  assert(lhs_opr.isReg() && des_opr.getReg() == lhs_opr.getReg());
  MachineOperand& base_opr = I->getOperand(2);
  assert(base_opr.isReg());
  MachineOperand& scale_opr = I->getOperand(3);
  assert(scale_opr.isImm());
  MachineOperand& idx_opr = I->getOperand(4);
  assert(idx_opr.isReg());
  MachineOperand& off_opr = I->getOperand(5);
  assert(off_opr.isImm());
  MachineOperand& seg_opr = I->getOperand(6);
  assert(seg_opr.isReg());
  assert(I->getOperand(7).isReg() && I->getOperand(7).getReg()==X86::EFLAGS);

  IRB->SetInsertPoint(BB);

  Value* lhs_val = get_reg_val(lhs_opr.getReg());
  // read src
  Value* rhs_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), 64);

  // compute
  Value* result = IRB->CreateSub(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(ADD32ri8)
{
  Addri(BB, I, 8,32);
}

define_visit(ADD32rr)
{
  Addri(BB, I , 32, 32);
}

define_visit(ADD64ri8)
{
  Addri(BB, I , 8, 64);
}

define_visit(ADD64rm)
{
  Addrm(BB, I , 64, 64);
}

define_visit(ADD64ri32)
{
  Addri(BB, I , 32, 64);
}

define_visit(ADD64rr)
{
  Addri(BB, I , 64, 64);
}

define_visit(ADD64i32)
{
  assert(I->getNumOperands()==4 && "ADD64i32's opr's num is not 4");
  MachineOperand& rhs_opr = I->getOperand(0);
  assert(rhs_opr.isImm() && "opr 0(rhs imm) is not imm in IREmitter::visitADD64i32");
  MachineOperand& lhs_opr = I->getOperand(3);
  assert(lhs_opr.isReg() && "opr 3(lhs reg) is not reg in IREmitter::visitADD64i32");
  MachineOperand& des_opr = I->getOperand(1);
  assert(des_opr.isReg() && des_opr.getReg()==lhs_opr.getReg() && "opr 1(defed reg) is not used reg in IREmitter::visitADD64i32");
  assert(I->getOperand(2).isReg() && I->getOperand(2).getReg()==X86::EFLAGS && "opr 2(falgs) is not eflags in IREmitter::visitADD64i32");

  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_reg_val(lhs_opr.getReg());

  //read rhs
  Constant* rhs_val = get_imm_val(rhs_opr.getImm(), 32, 64);

  // compute
  Value* result = IRB->CreateAdd(lhs_val, rhs_val);

  // writeback
  store_reg_val(des_opr.getReg(), result);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}

define_visit(XADD32rm)
{
  assert(I->getNumOperands() == 6);
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
  assert(rhs_opr.isReg());

  IRB->SetInsertPoint(BB);

  //read lhs
  Value* lhs_val = get_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), 32);

  //read rhs
  Value* rhs_val = get_reg_val(rhs_opr.getReg());

  // compute
  Value* result = IRB->CreateAdd(lhs_val, rhs_val);

  // writeback
  store_mem_val(base_opr.getReg(), scale_opr.getImm(), idx_opr.getReg(), off_opr.getImm(), seg_opr.getReg(), result);
  store_reg_val(rhs_opr.getReg(), lhs_val);

  store_AF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_PF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_ZF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_SF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_CF_val(I->getOpcode(), lhs_val, rhs_val, result);
  store_OF_val(I->getOpcode(), lhs_val, rhs_val, result);
}