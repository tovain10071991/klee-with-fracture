define_visit(CALL64pcrel32)
{
  assert(I->getNumOperands()==2);
  MachineOperand& off_opr = I->getOperand(0);
  assert(off_opr.isImm());
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::RSP);

  IRB->SetInsertPoint(BB);
  LLVMContext* context = Dec->getContext();  

 // rsp = rsp -8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateSub(lhs_val, rhs_val));

  // mov %rip, (%rsp)
  // read %rip val
  Value* src_val = get_reg_val(X86::RIP);

  // compute
  Value* result = src_val;

  store_mem_val(BB, X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, result);

  // jmp target
  int64_t off = off_opr.getImm();
  int64_t target = get_load_addr(Dec->getDisassembler()->getDebugOffset(I->getDebugLoc()), Dec->getDisassembler()->getExecutable()->getFileName(), Dec->getDisassembler()->getCurrentSectionName()) + I->getDesc().getSize() + off;
  Function* target_func = Dec->getFunctionByAddr(target);
  if(target_func)
    IRB->CreateCall(target_func);
  else
  {
    const object::SectionRef sec = Dec->getDisassembler()->getSectionByAddress(target);
    StringRef sec_name;
    if(error_code ec = sec.getName(sec_name))
      errx(-1, "get sec's name failed: %s", ec.message().c_str());
    if(sec_name.equals(".plt"))
    {
      std::string func_name = get_func_name_in_plt(target);
      target_func = dyn_cast<Function>(Dec->getModule()->getOrInsertFunction(func_name, FunctionType::get(Type::getVoidTy(*context), false)));
      IRB->CreateCall(target_func);
    }
    else
      IRB->CreateUnreachable();    
  }
}

define_visit(CALL64r)
{
  assert(I->getNumOperands()==2 && "CALL64r's opr's num is not 2");
  MachineOperand& target_opr = I->getOperand(0);
  assert(target_opr.isReg() && "opr 0(target) is not Reg in IREmitter::visitCALL64r");
  assert(I->getOperand(1).isReg() && I->getOperand(1).getReg()==X86::RSP && "opr 1(rsp) is not rsp in IREmitter::visitCALL64r");

  IRB->SetInsertPoint(BB);

 // rsp = rsp -8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateSub(lhs_val, rhs_val));

  // mov %rip, (%rsp)
  // read %rip val
  Value* src_val = get_reg_val(X86::RIP);

  // compute
  Value* result = src_val;

  store_mem_val(BB, X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, result);

 // jmp target
  Value* target_val = get_reg_val(target_opr.getReg());
  IRB->CreateCall(BB->getParent()->getParent()->getFunction("saib_collect_indirect"), target_val);
}

define_visit(CALL64m)
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
  assert(I->getOperand(5).isReg() && I->getOperand(5).getReg()==X86::RSP);

  IRB->SetInsertPoint(BB);

 // rsp = rsp -8
  Value* lhs_val = get_reg_val(X86::RSP);
  Constant* rhs_val = get_imm_val(8, 64, 64);
  store_reg_val(X86::RSP, IRB->CreateSub(lhs_val, rhs_val));

  // mov %rip, (%rsp)
  // read %rip val
  Value* src_val = get_reg_val(X86::RIP);

  // compute
  Value* result = src_val;

  store_mem_val(BB, X86::RSP, 0, X86::NoRegister, 0, X86::NoRegister, result);

 // jmp target
//  Value* target_val = get_reg_val(target_opr.getReg());

  IRB->CreateUnreachable();
}