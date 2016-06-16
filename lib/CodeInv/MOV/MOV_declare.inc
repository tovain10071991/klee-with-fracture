#define declare_mov(name) \
  void Mov##name(BasicBlock *BB, MachineInstr* I, unsigned init_size, unsigned final_size)
  declare_mov(r_);
  declare_mov(rm);
  declare_mov(mr);

declare_visit(MOV32r);
declare_visit(MOV32rm);
declare_visit(MOV64r);
declare_visit(MOV64ri32);
declare_visit(MOV8m);
declare_visit(MOV8ri);
declare_visit(MOV32m);
declare_visit(MOV64m);
declare_visit(MOV64mi32);
declare_visit(MOV64rm);

// declare_visit(MOVZX16rr8);
// declare_visit(MOVZX32rr8);
// declare_visit(MOVZX32rr16);
// declare_visit(MOVZX16rm8);
// declare_visit(MOVZX32rm8);
// declare_visit(MOVZX32rm16);
// declare_visit(MOVSX16rr8);
// declare_visit(MOVSX32rr16);
// declare_visit(MOVSX32rr8);
// declare_visit(MOVSX64rr8);
// declare_visit(MOVSX64rr16);
declare_visit(MOVSX64rr32);
// declare_visit(MOVSX16rm8);
// declare_visit(MOVSX32rm8);
// declare_visit(MOVSX32rm16);
// declare_visit(MOVSX64rm8);
// declare_visit(MOVSX64rm16);
// declare_visit(MOVSX64rm32);

// SSE
declare_visit(MOVUPSrm);
declare_visit(MOVUPSmr);
// declare_visit(MOVUPSrr);
// declare_visit(MOVUPSrr_REV);