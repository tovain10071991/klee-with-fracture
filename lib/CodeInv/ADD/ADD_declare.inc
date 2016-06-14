void ADDr(BasicBlock *BB, MachineInstr* I, unsigned init_size, unsigned final_size);
declare_visit(ADD32rr);
declare_visit(ADD64rr);
declare_visit(ADD32ri8);
declare_visit(ADD64ri8);
declare_visit(ADD64ri32);
declare_visit(ADD64i32);