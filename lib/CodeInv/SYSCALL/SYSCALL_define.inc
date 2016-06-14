define_visit(SYSCALL)
{
  IRB->SetInsertPoint(BB);

  Value* sys_num = get_reg_val(X86::RAX);
  Value* arg1 = get_reg_val(X86::RDI);
  Value* arg2 = get_reg_val(X86::RSI);
  Value* arg3 = get_reg_val(X86::RDX);
  Value* arg4 = get_reg_val(X86::R10);
  Value* arg5 = get_reg_val(X86::R8);
  Value* arg6 = get_reg_val(X86::R9);
  std::vector<Value*> args = {sys_num, arg1, arg2, arg3, arg4, arg5, arg6};

  Value* result = IRB->CreateCall(BB->getParent()->getParent()->getFunction("saib_syscall"), args);
  store_reg_val(X86::RAX, result);
}